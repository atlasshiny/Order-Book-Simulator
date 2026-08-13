#include <iostream>
#include "risk/RiskManager.hpp"

bool RiskManager::checkOrder(const Order& order, const Portfolio& portfolio) {
    // Basic risk checks
    if (order.quantity > MAX_ORDER_QUANTITY) {
        std::cout << "Risk Check Failed: Order quantity exceeds maximum allowed." << std::endl;
        return false; // Reject the order
    }

    if (order.quantity <= 0) {
        std::cout << "Risk Check Failed: Order quantity must be positive or non-zero." << std::endl;
        return false; // Reject the order
    }

    if (order.price <= 0) {
        std::cout << "Risk Check Failed: Order price must be positive or non-zero." << std::endl;
        return false; // Reject the order
    }

    if (order.type == OrderType::NONE) {
        std::cout << "Risk Check Failed: Order type is NONE, which is invalid." << std::endl;
        return false; // Reject the order
    }

    if (order.direction != OrderDirection::BUY && order.direction != OrderDirection::SELL) {
        std::cout << "Risk Check Failed: Order direction is invalid." << std::endl;
        return false; // Reject the order
    }

    // Portfolio-level checks
    if (order.direction == OrderDirection::BUY) {
        double order_cost = order.quantity * order.price;
        if (order_cost > portfolio.cash) {
            std::cout << "Risk Check Failed: Insufficient cash. Required: " << order_cost 
                      << ", Available: " << portfolio.cash << std::endl;
            return false;
        }
    }

    if (order.direction == OrderDirection::SELL) {
        auto it = portfolio.positions.find(order.symbol);
        double current_shares = (it != portfolio.positions.end()) ? it->second.quantity : 0.0;
        
        if (current_shares < order.quantity) {
            std::cout << "Risk Check Failed: Unbacked short selling not allowed for symbol. Held: " 
                      << current_shares << ", Attempted Sell: " << order.quantity << std::endl;
            return false;
        }
    }

    return true; // Accept the order if all checks pass
}