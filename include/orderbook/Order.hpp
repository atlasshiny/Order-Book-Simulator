#pragma once
#include <array>
#include <cstdint>
#include <array>

// all special "types" needed for an order
// order direction (BUY/SELL)
enum class OrderDirection {
    BUY,
    SELL
};

// order status (NEW, PARTIALLY_FILLED, FILLED, DONE_FOR_DAY, CANCELLED)
enum class OrderStatus {
    NEW,
    PARTIALLY_FILLED,
    FILLED,
    DONE_FOR_DAY, // Added to allign with FIX OrdStatus values
    CANCELLED
};

// order type (LIMIT, MARKET, IOC, POST_ONLY)
enum class OrderType {
    NONE = 0,
    LIMIT,
    MARKET,
    IOC,
    POST_ONLY
};

// the order itself, a structure holding all relevant information
// and the main object that the program will interact with.
struct Order { 
    OrderType type;
    OrderDirection direction;
    int price;
    int originalQuantity; // The original quantity of the order when it was placed
    int quantity; // The remaining quantity of the order that is yet to be filled
    uint64_t timestamp; // Time in nanoseconds since order creation, used for time priority in matching
    int clientID;
    OrderStatus status = OrderStatus::NEW; // Default status is NEW (this maps to FIX OrdStatus)
    int id; // Unique order ID assigned by the matching engine
    std::array<char, 8> symbol; // Symbol for the order, fixed size array of 8 characters
};