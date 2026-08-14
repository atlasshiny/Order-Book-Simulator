#pragma once
#include <deque>
#include <optional>
#include "Order.hpp"

class IExecutionListener; // Forward declaration

class OrderBook {
private:
    std::deque<Order> bidOrders;
    std::deque<Order> askOrders;
    int nextOrderId = 1;
    void matchAgainstBook(Order& incomingOrder, std::deque<Order>& oppositeBook);

    // Execution listener pointer for callbacks
    IExecutionListener* listener_ = nullptr;

public:
    // Core functionalities
    void placeLimitOrder(Order& order);
    void placeMarketOrder(Order& order);
    void placeImmediateOrCancelOrder(Order& order);
    void placePostOnlyOrder(Order& order);
    void matchOrders();
    std::optional<Order> cancelOrderAndReturn(int orderId);
    int getNextOrderId();

    // Market data retrieval
    void level1Data() const;
    void level2Data() const;

    // Orchestrator management
    void set_listener(IExecutionListener* listener);
    void remove_listener();
};
