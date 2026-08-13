#pragma once
#include "orderbook/Order.hpp"
#include "portfolio/Portfolio.hpp"

class RiskManager {
private:
    const int MAX_ORDER_QUANTITY = 1000;
    const int ORDER_PER_SECOND_LIMIT = 100; // When ASIO is implemented, ensure that the number of orders per second does not exceed this limit
public:
    bool checkOrder(const Order& order, const Portfolio& portfolio);
};
