#include <gtest/gtest.h>
#include "portfolio/PortfolioManager.hpp"
#include "portfolio/Portfolio.hpp"
#include "orderbook/Symbol.hpp"

class PortfolioManagerTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Setup initial state or common variables if needed
    }

    void TearDown() override {
        // Cleanup if needed
    }
};

// Test initializing a client portfolio and checking initial cash
TEST_F(PortfolioManagerTest, InitialPortfolioState) {
    PortfolioManager pm;
    int clientID = 1;

    pm.addPortfolio(clientID);
    pm.updateCash(clientID, 10000.0);

    const auto& portfolio = pm.getPortfolio(clientID);
    EXPECT_DOUBLE_EQ(portfolio.cash, 10000.0);
    EXPECT_TRUE(portfolio.positions.empty());
}

// Test opening a brand new long position
TEST_F(PortfolioManagerTest, OpenLongPosition) {
    PortfolioManager pm;
    int clientID = 1;
    pm.addPortfolio(clientID);

    orderbook::Symbol aapl = orderbook::make_symbol("AAPL");

    // Buy 10 shares at $150.00
    pm.updatePosition(clientID, aapl, 10.0, 150.0);
    pm.updateCash(clientID, -1500.0);

    const auto& portfolio = pm.getPortfolio(clientID);
    EXPECT_DOUBLE_EQ(portfolio.cash, -1500.0);
    ASSERT_EQ(portfolio.positions.count(aapl), 1);
    
    const auto& pos = portfolio.positions.at(aapl);
    EXPECT_DOUBLE_EQ(pos.quantity, 10.0);
    EXPECT_DOUBLE_EQ(pos.average_price, 150.0); // Assuming your updated function passes price
}

// Test scaling into a position and verifying weighted average cost basis
TEST_F(PortfolioManagerTest, WeightedAveragePriceCalculation) {
    PortfolioManager pm;
    int clientID = 1;
    pm.addPortfolio(clientID);

    orderbook::Symbol tsla = orderbook::make_symbol("TSLA");

    // First fill: Buy 10 shares at $200.00 -> Total Cost: $2,000
    pm.updatePosition(clientID, tsla, 10.0, 200.0);
    
    // Second fill: Buy 10 shares at $220.00 -> Total Cost: $2,200
    // Cumulative: 20 shares, Total Cost: $4,200 -> Avg Price: $210.00
    pm.updatePosition(clientID, tsla, 10.0, 220.0);

    const auto& portfolio = pm.getPortfolio(clientID);
    const auto& pos = portfolio.positions.at(tsla);
    
    EXPECT_DOUBLE_EQ(pos.quantity, 20.0);
    EXPECT_DOUBLE_EQ(pos.average_price, 210.0);
}

// Test reducing an existing position
TEST_F(PortfolioManagerTest, ReducePosition) {
    PortfolioManager pm;
    int clientID = 1;
    pm.addPortfolio(clientID);

    orderbook::Symbol msft = orderbook::make_symbol("MSFT");

    // Buy 50 shares at $300.00
    pm.updatePosition(clientID, msft, 50.0, 300.0);

    // Sell 20 shares (negative quantity fill)
    pm.updatePosition(clientID, msft, -20.0, 310.0);

    const auto& portfolio = pm.getPortfolio(clientID);
    const auto& pos = portfolio.positions.at(msft);

    EXPECT_DOUBLE_EQ(pos.quantity, 30.0);
    // Depending on your reduction logic, average price typically remains the cost basis of the open remainder
    EXPECT_DOUBLE_EQ(pos.average_price, 300.0); 
}