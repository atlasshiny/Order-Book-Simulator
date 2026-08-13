#include <gtest/gtest.h>
#include "orderbook/OrderBookManager.hpp"
#include "orderbook/Symbol.hpp"

TEST(OrderBookManagerTest, GetUnregisteredSymbolReturnsNullptr) {
    OrderBookManager manager;
    orderbook::Symbol aapl = orderbook::make_symbol("AAPL");

    EXPECT_EQ(manager.getOrderBook(aapl), nullptr);
}

TEST(OrderBookManagerTest, AddThenGetReturnsValidPointer) {
    OrderBookManager manager;
    orderbook::Symbol aapl = orderbook::make_symbol("AAPL");

    manager.addOrderBook(aapl);
    OrderBook* book = manager.getOrderBook(aapl);

    ASSERT_NE(book, nullptr);
}

TEST(OrderBookManagerTest, GetSameSymbolReturnsSameInstance) {
    OrderBookManager manager;
    orderbook::Symbol aapl = orderbook::make_symbol("AAPL");
    manager.addOrderBook(aapl);

    OrderBook* first = manager.getOrderBook(aapl);
    OrderBook* second = manager.getOrderBook(aapl);

    EXPECT_EQ(first, second);
}

TEST(OrderBookManagerTest, DifferentSymbolsGetIndependentBooks) {
    OrderBookManager manager;
    orderbook::Symbol aapl = orderbook::make_symbol("AAPL");
    orderbook::Symbol nvda = orderbook::make_symbol("NVDA");
    manager.addOrderBook(aapl);
    manager.addOrderBook(nvda);

    OrderBook* aaplBook = manager.getOrderBook(aapl);
    OrderBook* nvdaBook = manager.getOrderBook(nvda);

    ASSERT_NE(aaplBook, nullptr);
    ASSERT_NE(nvdaBook, nullptr);
    EXPECT_NE(aaplBook, nvdaBook);

    Order aaplOrder{OrderType::LIMIT, OrderDirection::BUY, 100, 10, 10, 1, 1, OrderStatus::NEW, 0};
    aaplOrder.id = aaplBook->getNextOrderId();
    aaplBook->placeLimitOrder(aaplOrder);

    // NVDA book's own next ID sequence should be unaffected by AAPL's book activity
    int nvdaFirstId = nvdaBook->getNextOrderId();
    EXPECT_EQ(nvdaFirstId, 1);
}

TEST(OrderBookManagerTest, RemoveOrderBookMakesSymbolUnavailable) {
    OrderBookManager manager;
    orderbook::Symbol aapl = orderbook::make_symbol("AAPL");
    manager.addOrderBook(aapl);
    ASSERT_NE(manager.getOrderBook(aapl), nullptr);

    manager.removeOrderBook(aapl);

    EXPECT_EQ(manager.getOrderBook(aapl), nullptr);
}

TEST(OrderBookManagerTest, RemoveNonexistentSymbolDoesNotCrash) {
    OrderBookManager manager;
    orderbook::Symbol ghost = orderbook::make_symbol("GHOST");

    EXPECT_NO_THROW(manager.removeOrderBook(ghost));
}

TEST(OrderBookManagerTest, AddingSameSymbolTwiceDoesNotThrow) {
    OrderBookManager manager;
    orderbook::Symbol aapl = orderbook::make_symbol("AAPL");
    manager.addOrderBook(aapl);

    EXPECT_NO_THROW(manager.addOrderBook(aapl));
}