#include "orderbook/OrderBookManager.hpp"

void OrderBookManager::addOrderBook(const char* symbol) {
    orderBooks.emplace(symbol, OrderBook());
}

void OrderBookManager::removeOrderBook(const char* symbol) {
    orderBooks.erase(symbol);
}

OrderBook* OrderBookManager::getOrderBook(const char* symbol) {
    auto it = orderBooks.find(symbol);
    if (it != orderBooks.end()) {
        return &(it->second);
    }
    
    return nullptr; // Return nullptr if the order book for the symbol doesn't exist
}