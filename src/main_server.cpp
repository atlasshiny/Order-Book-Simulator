#include "network/TCPServer.hpp"
#include "orchestrator/ExchangeOrchestrator.hpp"
#include "gateways/FIXGateway.hpp"
#include "orderbook/Symbol.hpp"
#include <string>
#include <memory>
#include <iostream>

int main() {
    try {
        unsigned short port = 8080;
        std::cout << "[Main] Initializing Order Book Simulator on port " << port << "...\n";

        // Create the business logic component
        auto gateway = std::make_unique<FIXGateway>();
        auto orchestrator = std::make_shared<ExchangeOrchestrator>(std::move(gateway));

        // Add order books for specific symbols
        orderbook::Symbol AAPL_symbol = orderbook::make_symbol("AAPL");
        orderbook::Symbol NVDA_symbol = orderbook::make_symbol("NVDA");

        orchestrator->addOrderBook(orderbook::Symbol(AAPL_symbol));
        orchestrator->addOrderBook(orderbook::Symbol(NVDA_symbol));

        // Start the server (this hooks up the internal io_context acceptor loop)
        TCPServer server(port, orchestrator);

        // This blocks and keeps the engine alive running your async events!
        server.run();

    } catch (const std::exception& e) {
        std::cerr << "[Main] Runtime exception: " << e.what() << "\n";
    }
    return 0;
}