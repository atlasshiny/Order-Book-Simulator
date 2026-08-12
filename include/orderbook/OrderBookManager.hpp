#pragma once
#include <unordered_map>
#include "orderbook/OrderBook.hpp"

class OrderBookManager {
public:
    // Order book management
    void addOrderBook(const char* symbol);
    void removeOrderBook(const char* symbol);
    OrderBook* getOrderBook(const char* symbol);

private:
    std::unordered_map<const char*, OrderBook> orderBooks;
};