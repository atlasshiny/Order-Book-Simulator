#pragma once
#include <unordered_map>
#include "orderbook/Symbol.hpp"

struct Position {
    double quantity;
    double average_price;
};

struct Portfolio {
    double cash; // total cash in the portfolio

    // current positions in the portfolio, keyed by symbol
    std::unordered_map<orderbook::Symbol, Position, orderbook::SymbolHash> positions;
};