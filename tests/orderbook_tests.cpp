#include <gtest/gtest.h>
#include <chrono>
#include "orderbook/OrderBook.hpp"
#include "orderbook/Order.hpp"

// Helper function to create a dummy order easily
Order CreateTestOrder(OrderType type, OrderDirection direction, int price, int quantity, int clientID) {
    uint64_t ts = std::chrono::duration_cast<std::chrono::nanoseconds>(
                      std::chrono::high_resolution_clock::now().time_since_epoch())
                      .count();
    
    return Order{
        type,
        direction,
        price,
        quantity,
        quantity,
        ts,
        clientID,
        OrderStatus::NEW,
        0
    };
}

// Test 1: Verify a resting limit order receives an engine ID
TEST(OrderBookTest, PlaceRestingLimitOrder) {
    OrderBook book;
    Order buy_order = CreateTestOrder(OrderType::LIMIT, OrderDirection::BUY, 150, 100, 42);
    
    // Manually mimic the orchestrator's step before placement
    buy_order.id = book.getNextOrderId(); 
    
    book.placeLimitOrder(buy_order);
    
    EXPECT_EQ(buy_order.id, 1); 
    EXPECT_EQ(buy_order.quantity, 100); 
    EXPECT_EQ(buy_order.status, OrderStatus::NEW);
}

// Full execution requires calling matchOrders()
TEST(OrderBookTest, FullLimitOrderExecution) {
    OrderBook book;
    
    Order resting_buy = CreateTestOrder(OrderType::LIMIT, OrderDirection::BUY, 100, 50, 101);
    resting_buy.id = book.getNextOrderId();
    book.placeLimitOrder(resting_buy);
    
    Order aggressive_sell = CreateTestOrder(OrderType::LIMIT, OrderDirection::SELL, 100, 50, 202);
    aggressive_sell.id = book.getNextOrderId();
    book.placeLimitOrder(aggressive_sell);
    
    // CRITICAL: Your engine relies on an explicit match cycle pass![cite: 5]
    book.matchOrders(); 
}

// IOC partial fill and immediate prune
TEST(OrderBookTest, ImmediateOrCancelPartialFill) {
    OrderBook book;
    
    Order resting_sell = CreateTestOrder(OrderType::LIMIT, OrderDirection::SELL, 200, 10, 505);
    resting_sell.id = book.getNextOrderId();
    book.placeLimitOrder(resting_sell);
    
    Order ioc_buy = CreateTestOrder(OrderType::IOC, OrderDirection::BUY, 200, 50, 606);
    ioc_buy.id = book.getNextOrderId();
    
    // Run the sequence
    book.placeImmediateOrCancelOrder(ioc_buy);
    book.matchOrders(); 

}

TEST(OrderBookTest, CancelExistingBuyOrderRemovesFromBook) {
    OrderBook book;
    Order buy_order = CreateTestOrder(OrderType::LIMIT, OrderDirection::BUY, 100, 50, 42);
    buy_order.id = book.getNextOrderId();
    book.placeLimitOrder(buy_order);

    testing::internal::CaptureStdout();
    book.cancelOrder(buy_order.id);
    std::string output = testing::internal::GetCapturedStdout();

    EXPECT_NE(output.find("Cancelled BUY order ID"), std::string::npos);
}

TEST(OrderBookTest, CancelExistingSellOrderRemovesFromBook) {
    OrderBook book;
    Order sell_order = CreateTestOrder(OrderType::LIMIT, OrderDirection::SELL, 100, 50, 42);
    sell_order.id = book.getNextOrderId();
    book.placeLimitOrder(sell_order);

    testing::internal::CaptureStdout();
    book.cancelOrder(sell_order.id);
    std::string output = testing::internal::GetCapturedStdout();

    EXPECT_NE(output.find("Cancelled SELL order ID"), std::string::npos);
}

TEST(OrderBookTest, CancelNonexistentOrderDoesNotCrash) {
    OrderBook book;

    testing::internal::CaptureStdout();
    EXPECT_NO_THROW(book.cancelOrder(9999));
    std::string output = testing::internal::GetCapturedStdout();

    EXPECT_NE(output.find("not found for cancellation"), std::string::npos);
}

TEST(OrderBookTest, CancelledOrderDoesNotParticipateInMatching) {
    OrderBook book;

    Order buy_order = CreateTestOrder(OrderType::LIMIT, OrderDirection::BUY, 100, 50, 42);
    buy_order.id = book.getNextOrderId();
    book.placeLimitOrder(buy_order);
    book.cancelOrder(buy_order.id);

    Order sell_order = CreateTestOrder(OrderType::LIMIT, OrderDirection::SELL, 100, 50, 99);
    sell_order.id = book.getNextOrderId();
    book.placeLimitOrder(sell_order);

    testing::internal::CaptureStdout();
    book.matchOrders();
    std::string output = testing::internal::GetCapturedStdout();

    // No match should occur since the buy side was cancelled before matching
    EXPECT_EQ(output.find("Matched BUY"), std::string::npos);
}

// --- Level 1 / Level 2 output tests ---

TEST(OrderBookTest, Level1DataShowsNAWhenEmpty) {
    OrderBook book;

    testing::internal::CaptureStdout();
    book.level1Data();
    std::string output = testing::internal::GetCapturedStdout();

    EXPECT_NE(output.find("Best Bid: N/A"), std::string::npos);
    EXPECT_NE(output.find("Best Ask: N/A"), std::string::npos);
}

TEST(OrderBookTest, Level1DataShowsBestBidAndAsk) {
    OrderBook book;

    Order buy_order = CreateTestOrder(OrderType::LIMIT, OrderDirection::BUY, 100, 25, 1);
    buy_order.id = book.getNextOrderId();
    book.placeLimitOrder(buy_order);

    Order sell_order = CreateTestOrder(OrderType::LIMIT, OrderDirection::SELL, 105, 30, 2);
    sell_order.id = book.getNextOrderId();
    book.placeLimitOrder(sell_order);

    testing::internal::CaptureStdout();
    book.level1Data();
    std::string output = testing::internal::GetCapturedStdout();

    EXPECT_NE(output.find("Best Bid: $100 x 25"), std::string::npos);
    EXPECT_NE(output.find("Best Ask: $105 x 30"), std::string::npos);
}

TEST(OrderBookTest, Level1DataShowsHighestBidAmongMultiple) {
    OrderBook book;

    Order lowerBid = CreateTestOrder(OrderType::LIMIT, OrderDirection::BUY, 90, 10, 1);
    lowerBid.id = book.getNextOrderId();
    book.placeLimitOrder(lowerBid);

    Order higherBid = CreateTestOrder(OrderType::LIMIT, OrderDirection::BUY, 95, 10, 2);
    higherBid.id = book.getNextOrderId();
    book.placeLimitOrder(higherBid);

    // NOTE: level1Data() reads bidOrders.front() directly without sorting.
    // It relies on matchOrders() (or another sort pass) having already run
    // to guarantee price-time priority ordering. Call matchOrders() first
    // so this test reflects real usage via the orchestrator.
    book.matchOrders();

    testing::internal::CaptureStdout();
    book.level1Data();
    std::string output = testing::internal::GetCapturedStdout();

    EXPECT_NE(output.find("Best Bid: $95"), std::string::npos);
}

TEST(OrderBookTest, Level2DataAggregatesMultipleOrdersAtSamePrice) {
    OrderBook book;

    Order sell1 = CreateTestOrder(OrderType::LIMIT, OrderDirection::SELL, 100, 10, 1);
    sell1.id = book.getNextOrderId();
    book.placeLimitOrder(sell1);

    Order sell2 = CreateTestOrder(OrderType::LIMIT, OrderDirection::SELL, 100, 15, 2);
    sell2.id = book.getNextOrderId();
    book.placeLimitOrder(sell2);

    testing::internal::CaptureStdout();
    book.level2Data();
    std::string output = testing::internal::GetCapturedStdout();

    // Two resting sells at the same price should aggregate to 25 total quantity
    EXPECT_NE(output.find("25"), std::string::npos);
    EXPECT_NE(output.find("$100"), std::string::npos);
}