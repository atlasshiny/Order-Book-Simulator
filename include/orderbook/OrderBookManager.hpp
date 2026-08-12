#pragma once
#include <unordered_map>
#include "orderbook/Symbol.hpp"
#include "orderbook/OrderBook.hpp"

class OrderBookManager {
public:
    // Order book management
    void addOrderBook(const orderbook::Symbol& symbol);
    void removeOrderBook(const orderbook::Symbol& symbol);
    OrderBook* getOrderBook(const orderbook::Symbol& symbol);

private:
    std::unordered_map<orderbook::Symbol, OrderBook, orderbook::SymbolHash> orderBooks_;
};