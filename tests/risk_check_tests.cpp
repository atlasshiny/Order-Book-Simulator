#include <gtest/gtest.h>
#include "risk/RiskManager.hpp"
#include "portfolio/Portfolio.hpp"
#include "orderbook/Symbol.hpp"

TEST(RiskManagerTest, OrderValidation) {
    RiskManager riskManager;
    Portfolio portfolio;
    portfolio.cash = 500000.0;

    // Test a valid order
    Order validOrder{OrderType::LIMIT, OrderDirection::BUY, 100, 10, 10, 1234567890, 1, OrderStatus::NEW, 0};
    EXPECT_TRUE(riskManager.checkOrder(validOrder, portfolio));
}

TEST(RiskManagerTest, ZeroQuantityOrder) {
    RiskManager riskManager;
    Portfolio portfolio;
    portfolio.cash = 500000.0;

    // Test an order with zero quantity
    Order zeroQuantityOrder{OrderType::LIMIT, OrderDirection::BUY, 100, 0, 0, 1234567890, 1, OrderStatus::NEW, 0};
    EXPECT_FALSE(riskManager.checkOrder(zeroQuantityOrder, portfolio));
}

TEST(RiskManagerTest, NegativePriceOrder) {
    RiskManager riskManager;
    Portfolio portfolio;
    portfolio.cash = 500000.0;

    // Test an order with negative price
    Order negativePriceOrder{OrderType::LIMIT, OrderDirection::BUY, -100, 10, 10, 1234567890, 1, OrderStatus::NEW, 0};
    EXPECT_FALSE(riskManager.checkOrder(negativePriceOrder, portfolio));
    
}

TEST(RiskManagerTest, InvalidOrderType) {
    RiskManager riskManager;
    Portfolio portfolio;
    portfolio.cash = 500000.0;

    // Test an order with invalid type
    Order invalidTypeOrder{OrderType::NONE, OrderDirection::BUY, 100, 10, 10, 1234567890, 1, OrderStatus::NEW, 0};
    EXPECT_FALSE(riskManager.checkOrder(invalidTypeOrder, portfolio));
}

TEST(RiskManagerTest, InvalidOrderDirection) {
    RiskManager riskManager;
    Portfolio portfolio;
    portfolio.cash = 500000.0;

    // Test an order with invalid direction
    Order invalidDirectionOrder{OrderType::LIMIT, static_cast<OrderDirection>(-1), 100, 10, 10, 1234567890, 1, OrderStatus::NEW, 0};
    EXPECT_FALSE(riskManager.checkOrder(invalidDirectionOrder, portfolio));
}

TEST(RiskManagerTest, BuyOrderPassesWhenSufficientCash) {
    RiskManager riskManager;
    Portfolio portfolio;
    portfolio.cash = 500000.0;

    Order buyOrder;
    buyOrder.direction = OrderDirection::BUY;
    buyOrder.quantity = 10.0;
    buyOrder.price = 150.0;
    buyOrder.symbol = orderbook::make_symbol("AAPL");;
    
    bool allowed = riskManager.checkOrder(buyOrder, portfolio);
    EXPECT_TRUE(allowed);
}

TEST(RiskManagerTest, BuyOrderFailsWhenInsufficientCash) {
    RiskManager riskManager;
    Portfolio portfolio;
    portfolio.cash = 10000.0;

    Order buyOrder;
    buyOrder.direction = OrderDirection::BUY;
    buyOrder.quantity = 100.0; // 100 shares * $150 = $15,000
    buyOrder.symbol = orderbook::make_symbol("AAPL");

    double estimated_price = 150.0; 
    
    bool allowed = riskManager.checkOrder(buyOrder, portfolio);
    EXPECT_FALSE(allowed); // Portfolio only has $10,000 cash
}

TEST(RiskManagerTest, SellOrderFailsWhenUnownedShort) {
    RiskManager riskManager;
    Portfolio portfolio;

    Order sellOrder;
    sellOrder.direction = OrderDirection::SELL;
    sellOrder.quantity = 10.0;
    sellOrder.symbol = orderbook::make_symbol("AAPL");

    // Portfolio has 0 shares of AAPL
    bool allowed = riskManager.checkOrder(sellOrder, portfolio);
    EXPECT_FALSE(allowed);
}

TEST(RiskManagerTest, SellOrderPassesWhenOwningShares) {
    RiskManager riskManager;
    Portfolio portfolio;
    portfolio.cash = 500000.0;

    // Populate portfolio with existing position
    portfolio.positions[orderbook::make_symbol("AAPL")] = Position{20.0, 140.0};

    Order sellOrder;
    sellOrder.direction = OrderDirection::SELL;
    sellOrder.quantity = 10.0; // Selling 10 out of 20 owned shares
    sellOrder.price = 150.0;
    sellOrder.symbol = orderbook::make_symbol("AAPL");

    bool allowed = riskManager.checkOrder(sellOrder, portfolio);
    EXPECT_TRUE(allowed);
}