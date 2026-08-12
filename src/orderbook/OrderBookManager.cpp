#include "orderbook/OrderBookManager.hpp"

void OrderBookManager::addOrderBook(const orderbook::Symbol& symbol) {
    orderBooks_.emplace(symbol, OrderBook());
}

void OrderBookManager::removeOrderBook(const orderbook::Symbol& symbol) {
    orderBooks_.erase(symbol);
}

OrderBook* OrderBookManager::getOrderBook(const orderbook::Symbol& symbol) {
    auto it = orderBooks_.find(symbol);
    if (it != orderBooks_.end()) {
        return &(it->second);
    }
    
    return nullptr; // Return nullptr if the order book for the symbol doesn't exist
}