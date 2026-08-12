#include "orderbook/OrderBookManager.hpp"

void OrderBookManager::addOrderBook(const std::array<char, 8>& symbol) {
    orderBooks.emplace(symbol, OrderBook());
}

void OrderBookManager::removeOrderBook(const std::array<char, 8>& symbol) {
    orderBooks.erase(symbol);
}

OrderBook* OrderBookManager::getOrderBook(const std::array<char, 8>& symbol) {
    auto it = orderBooks.find(symbol);
    if (it != orderBooks.end()) {
        return &(it->second);
    }
    
    return nullptr; // Return nullptr if the order book for the symbol doesn't exist
}