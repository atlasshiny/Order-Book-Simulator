#include "portfolio/PortfolioManager.hpp"
#include "portfolio/Portfolio.hpp"
#include <stdexcept>

void PortfolioManager::updateCash(int clientID, double amount) {
    portfolios_[clientID].cash += amount;
}

void PortfolioManager::updatePosition(int clientID, const orderbook::Symbol& symbol, double fill_quantity, double fill_price) {
    Position& pos = portfolios_[clientID].positions[symbol];

    if (pos.quantity == 0.0) {
        pos.quantity = fill_quantity;
        pos.average_price = fill_price;
    } else {
        double new_quantity = pos.quantity + fill_quantity;
        
        if (new_quantity == 0.0) {
            pos.quantity = 0.0;
            pos.average_price = 0.0;
        } else if ((pos.quantity > 0 && fill_quantity > 0) || (pos.quantity < 0 && fill_quantity < 0)) {
            // Adding to position: Recalculate weighted average price
            double total_cost = (pos.quantity * pos.average_price) + (fill_quantity * fill_price);
            pos.quantity = new_quantity;
            pos.average_price = total_cost / new_quantity;
        } else {
            // Reducing position: Quantity changes, but cost basis of remaining shares stays the same
            pos.quantity = new_quantity;
            // DO NOT change pos.average_price here!
        }
    }
}

void PortfolioManager::addPortfolio(int clientID) {
    portfolios_[clientID] = Portfolio();
}

const Portfolio& PortfolioManager::getPortfolio(int clientID) const {
    auto it = portfolios_.find(clientID);
    if (it != portfolios_.end()) {
        return it->second;
    } else {
        throw std::runtime_error("Portfolio for clientID " + std::to_string(clientID) + " not found.");
    }
}
