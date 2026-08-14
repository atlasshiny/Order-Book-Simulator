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
#include "parsers/FIXParser.hpp" 
#include "orchestrator/ExchangeOrchestrator.hpp"
#include "orderbook/Symbol.hpp"

int main() {
    // Instantiate the orchestrator with a cleanly allocated FIX gateway to avoid copy-constructor overhead/errors.
    ExchangeOrchestrator engine(std::make_unique<FIXGateway>());

    orderbook::Symbol aapl_symbol = orderbook::make_symbol("AAPL");
    engine.addOrderBook(aapl_symbol);

    // Instantiate the FIX "client" writer & parser for serialization/deserialization
    FIXWriter fixWriter;
    FIXParser fixParser;

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

        // Convert symbol string to fixed-size array
        orderbook::Symbol symbolArray = orderbook::make_symbol(symbol);

        // Safely initialize the Order struct explicitly to avoid brace-init misalignment
        Order consoleOrder{};
        consoleOrder.type = orderType;
        consoleOrder.direction = direction;
        consoleOrder.price = price;
        consoleOrder.quantity = quantity;
        consoleOrder.originalQuantity = quantity;
        consoleOrder.timestamp = current_time;
        consoleOrder.clientID = clientID;
        consoleOrder.clOrdID = clientID; 
        consoleOrder.status = OrderStatus::NEW;
        consoleOrder.id = 0;
        consoleOrder.symbol = symbolArray;
        
        // Initialize the client's portfolio with starting cash
        double starting_cash = 100000.0;
        engine.updatePortfolioCash(clientID, starting_cash);

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

        // Parse locally for validation before routing through the orchestrator ingress path
        std::optional<Order> parsedOrderOpt = fixParser.parse(rawWireMsg);
        
        if (!parsedOrderOpt.has_value()) {
            std::cout << "Error: Failed to parse the wire message." << std::endl;
            continue;
        }

        // Route through the public orchestrator entrypoint instead of private/internal processing APIs.
        engine.processConsoleOrder(consoleOrder);
    }
    return 0;
}