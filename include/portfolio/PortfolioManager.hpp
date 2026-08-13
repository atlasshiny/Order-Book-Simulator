#pragma once
#include "portfolio/Portfolio.hpp"
#include "orderbook/Symbol.hpp"
#include <unordered_map>

class PortfolioManager {
public:
    PortfolioManager() {};
    void updateCash(int clientID, double amount);
    void updatePosition(int clientID, const orderbook::Symbol& symbol, double fill_quantity, double fill_price);
    const Portfolio& getPortfolio(int clientID) const;
    void addPortfolio(int clientID);

private:
    std::unordered_map<int, Portfolio> portfolios_; // Keyed by clientID from the FIX message, each client has their own portfolio
};