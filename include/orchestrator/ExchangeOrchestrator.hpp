#pragma once
#include "orderbook/OrderBookManager.hpp"
#include "risk/RiskManager.hpp"
#include "gateways/IGateway.hpp"
#include "orderbook/Order.hpp" 
#include "gateways/FIXDefinition.hpp"
#include "orderbook/IExecutionListener.hpp"
#include "orderbook/Symbol.hpp"
#include <memory>
#include "portfolio/PortfolioManager.hpp"

class TCPSession; // Forward declaration

class ExchangeOrchestrator : public IExecutionListener {
public:
    // Constructor accepts which protocol gateway we want to use
    ExchangeOrchestrator(std::unique_ptr<IGateway> protocolGateway)
        : gateway_(std::move(protocolGateway)), riskManager_(), orderBookManager_(), portfolioManager_() {}

    // public-facing methods from the gateway to handle incoming raw wire messages
    std::optional<Order> on_data_received(std::shared_ptr<TCPSession> session, std::string_view raw_data); 

    // callback hooks
    void on_client_connect(std::shared_ptr<TCPSession> session);
    void on_client_disconnect(std::shared_ptr<TCPSession> session);
    void on_order_accepted(const Order& order);
    void on_order_executed(const Order& order, int price, int quantity) override;
    void on_order_canceled(const Order& order, std::shared_ptr<TCPSession> session);

    // order book management methods
    void addOrderBook(const orderbook::Symbol& symbol);
    void removeOrderBook(const orderbook::Symbol& symbol);

    // order cancellation method
    bool cancel_order(int orderId, const orderbook::Symbol& symbol, std::shared_ptr<TCPSession> session);

    // portfolio management methods
    void updatePortfolioCash(int clientID, double amount);

    // Output the current state of the order book in console (I/O blocking)
    void outputOrderBookState(const orderbook::Symbol& symbol) const;

private:
    void processOrder(std::shared_ptr<TCPSession> session, Order& order);

    std::shared_ptr<TCPSession> currentSession_ = nullptr; // current session for the incoming order

    // objects for the core components of the exchange
    std::unique_ptr<IGateway> gateway_; // Abstract boundary for protocol decoding
    RiskManager riskManager_; 
    OrderBookManager orderBookManager_; // Manages multiple order books for different symbols 
    PortfolioManager portfolioManager_; // Manages the portfolios of the exchange
};