#pragma once
#include <unordered_map>
#include <array>
#include "orderbook/OrderBook.hpp"

class OrderBookManager {
public:
    // Order book management
    void addOrderBook(const std::array<char, 8>& symbol);
    void removeOrderBook(const std::array<char, 8>& symbol);
    OrderBook* getOrderBook(const std::array<char, 8>& symbol);

private:
    std::unordered_map<std::array<char, 8>, OrderBook> orderBooks;
};