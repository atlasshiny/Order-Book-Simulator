#include <iostream>
#include <string>
#include <chrono>
#include <optional>
#include <string_view>

// Include your domain models and protocol components
#include "orderbook/Order.hpp" 
#include "gateways/FIXDefinition.hpp"
#include "gateways/FIXGateway.hpp"
#include "writers/FIXWriter.hpp"
#include "orchestrator/ExchangeOrchestrator.hpp"
#include "orderbook/Symbol.hpp"

int main() {
    // Instantiate the FIX gateway and orchestrator
    FIXGateway fixGateway;
    ExchangeOrchestrator engine(std::make_unique<FIXGateway>(fixGateway));

    // Instantiate the FIX "client" writer for serialization
    FIXWriter fixWriter;

    // Fixed-size stack allocation for the high-performance write buffer
    char wireBuffer[1024];

    while (true) {
        int price, quantity, clientID;
        uint64_t current_time;
        std::string orderTypeStr, directionStr, symbol;

        std::cout << "\nEnter (LIMIT/MARKET), (BUY/SELL), Price, Qty, ClientID, Symbol (or Ctrl+C to exit): ";
        if (!(std::cin >> orderTypeStr >> directionStr >> price >> quantity >> clientID >> symbol)) {
            break; 
        }

        // Determine order type
        OrderType orderType;
        if (orderTypeStr == "LIMIT") {
            orderType = OrderType::LIMIT;
        } else if (orderTypeStr == "MARKET") {
            orderType = OrderType::MARKET;
        } else {
            std::cout << "Invalid order type. Please enter LIMIT or MARKET." << std::endl;
            continue;
        }

        // Determine order direction
        OrderDirection direction;
        if (directionStr == "BUY") {
            direction = OrderDirection::BUY;
        } else if (directionStr == "SELL") {
            direction = OrderDirection::SELL;
        } else {
            std::cout << "Invalid direction. Please enter BUY or SELL." << std::endl;
            continue;
        }

        current_time = std::chrono::duration_cast<std::chrono::nanoseconds>(
            std::chrono::high_resolution_clock::now().time_since_epoch()
        ).count();

        // Create our local order template representing a client message
        // convert symbol string to fixed-size array
        orderbook::Symbol symbolArray = orderbook::make_symbol(symbol);

        Order consoleOrder{orderType, direction, price, quantity, quantity, current_time, clientID, OrderStatus::NEW, 0, symbolArray};

        // STEP 1: FIX WRITER SERIALIZATION (Client Sending Order)
        size_t bytesWritten = fixWriter.write(consoleOrder, wireBuffer, sizeof(wireBuffer));
        
        if (bytesWritten == 0) {
            std::cout << "Error: FIXWriter failed to serialize the message (buffer too small)." << std::endl;
            continue;
        }

        // Create a string view of the raw wire data
        std::string_view rawWireMsg(wireBuffer, bytesWritten);

        // Print the raw wire message to console (replacing SOH with '|' for visibility)
        std::cout << "--- [RAW FIX WIRE MESSAGE SENDING] ---" << std::endl;
        for (char c : rawWireMsg) {
            if (c == '\x01') std::cout << '|';
            else std::cout << c;
        }
        std::cout << "\n--------------------------------------" << std::endl;

        std::optional<Order> parsedOrderOpt = engine.on_data_received(nullptr, rawWireMsg); // Simulate receiving the message in the orchestrator
        if (!parsedOrderOpt.has_value()) {
            std::cout << "Error: Failed to parse the wire message." << std::endl;
            continue;
        }
        Order parsedOrder = parsedOrderOpt.value();
    }
    return 0;
}