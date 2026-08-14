#include <algorithm>
#include <iostream>
#include <map>
#include <iomanip>
#include "orderbook/OrderBook.hpp"
#include "orderbook/IExecutionListener.hpp"

void OrderBook::matchAgainstBook(Order& incomingOrder, std::deque<Order>& oppositeBook) {
    // Ensure the book is sorted by price/time priority before matching
    
    while (!oppositeBook.empty() && incomingOrder.quantity > 0) {
        Order& restingOrder = oppositeBook.front();

        // Determine price limit constraints
        // If it's a BUY order, it can only match if the resting SELL price <= incoming BUY price
        // If it's a SELL order, it can only match if the resting BUY price >= incoming SELL price
        if (incomingOrder.direction == OrderDirection::BUY && incomingOrder.price < restingOrder.price) break;
        if (incomingOrder.direction == OrderDirection::SELL && incomingOrder.price > restingOrder.price) break;

        // Match quantities
        int tradeQuantity = std::min(incomingOrder.quantity, restingOrder.quantity);
        
        // Update quantities
        incomingOrder.quantity -= tradeQuantity;
        restingOrder.quantity -= tradeQuantity;

        std::cout << "Trade Executed: " << tradeQuantity << " shares at $" << restingOrder.price << std::endl;

        // callback hook; Notify execution for BOTH sides of the aggressive sweep
        if (listener_) {
            listener_->on_order_executed(restingOrder, restingOrder.price, tradeQuantity);
            listener_->on_order_executed(incomingOrder, restingOrder.price, tradeQuantity);
        }

        if (restingOrder.quantity <= 0) {
            restingOrder.status = OrderStatus::FILLED;
            oppositeBook.pop_front();
        }
    }
}

void OrderBook::placeLimitOrder(Order& order) {
    if (order.direction == OrderDirection::BUY) {
        // Add limit buy order to bid side
        bidOrders.push_back(order);
    } else if (order.direction == OrderDirection::SELL) {
        // Add limit sell order to ask side
        askOrders.push_back(order);
    }

}

void OrderBook::placeMarketOrder(Order& order) {
    if (order.direction == OrderDirection::BUY) {
        // ensure best asks are sorted first (lowest price) for market orders
        sort(askOrders.begin(), askOrders.end(), [](const Order& a, const Order& b) {
            if (a.price == b.price) return a.timestamp < b.timestamp;
            return a.price < b.price;
        });

        matchAgainstBook(order, askOrders);

        // Post-execution report
        if (order.quantity > 0) {
            std::cout << "Market BUY ID " << order.id 
                      << " expired unfilled for " << order.quantity 
                      << " shares due to total lack of market liquidity." << std::endl;
        }
    } 
    else if (order.direction == OrderDirection::SELL) {
        // Ensure the bid book is sorted highest-price-first before sweeping
        sort(bidOrders.begin(), bidOrders.end(), [](const Order& a, const Order& b) {
            if (a.price == b.price) return a.timestamp < b.timestamp;
            return a.price > b.price;
        });

        matchAgainstBook(order, bidOrders);

        // Post-execution report
        if (order.quantity > 0) {
            std::cout << "Market SELL ID " << order.id 
                      << " expired unfilled for " << order.quantity 
                      << " shares due to total lack of market liquidity." << std::endl;
        }
    }

}

void OrderBook::placeImmediateOrCancelOrder(Order& order) {
    // Implementation for Immediate-Or-Cancel (IOC) orders
    // This function will attempt to fill the order immediately and cancel any unfilled portion
    // Similar logic to placeMarketOrder but with cancellation of remaining quantity

    if (order.direction == OrderDirection::BUY) {
        if (order.quantity > 0) {
            // ensure best asks are sorted first (lowest price) for market orders
            sort(askOrders.begin(), askOrders.end(), [](const Order& a, const Order& b) {
                if (a.price == b.price) return a.timestamp < b.timestamp;
                return a.price < b.price;
            });

            matchAgainstBook(order, askOrders);

            if (order.quantity > 0) {
                std::cout << "IOC BUY ID " << order.id 
                     << " cancelled for remaining " << order.quantity 
                     << " shares that could not be filled immediately." << std::endl;
            }
        } else {
            std::cout << "IOC BUY ID " << order.id 
                 << " cancelled immediately as it had zero quantity." << std::endl;
        }
    } else if (order.direction == OrderDirection::SELL) {
        if (order.quantity > 0) {
            // Ensure the bid book is sorted highest-price-first before sweeping
            sort(bidOrders.begin(), bidOrders.end(), [](const Order& a, const Order& b) {
                if (a.price == b.price) return a.timestamp < b.timestamp;
                return a.price > b.price;
            });

            matchAgainstBook(order, bidOrders);

            if (order.quantity > 0) {
                std::cout << "IOC SELL ID " << order.id 
                     << " cancelled for remaining " << order.quantity 
                     << " shares that could not be filled immediately." << std::endl;
            }
        } else {
            std::cout << "IOC SELL ID " << order.id 
                 << " cancelled immediately as it had zero quantity." << std::endl;
        }
    }

}

void OrderBook::placePostOnlyOrder(Order& order) {
    // Implementation for Post-Only orders
    // This function will only add the order to the book if it does not immediately match
    // If it would match, it should be rejected or cancelled

    if (order.direction == OrderDirection::BUY) {
        if (!askOrders.empty() && order.price >= askOrders.front().price) {
            std::cout << "Post-Only BUY ID " << order.id 
                 << " rejected as it would immediately match with existing SELL orders." << std::endl;
            return; // Do not add to book
        }
        // Add limit buy order to bid side
        bidOrders.push_back(order);
    } else if (order.direction == OrderDirection::SELL) {
        if (!bidOrders.empty() && order.price <= bidOrders.front().price) {
            std::cout << "Post-Only SELL ID " << order.id 
                 << " rejected as it would immediately match with existing BUY orders." << std::endl;
            return; // Do not add to book
        }
        // Add limit sell order to ask side
        askOrders.push_back(order);
    }

}

void OrderBook::matchOrders() {
    // sort best bids first (highest price) and best asks first (lowest price)
    sort(bidOrders.begin(), bidOrders.end(), [](const Order& a, const Order& b) {
        if (a.price == b.price) {
            return a.timestamp < b.timestamp; // earlier orders have priority
        }
        return a.price > b.price;
    });
    sort(askOrders.begin(), askOrders.end(), [](const Order& a, const Order& b) {
        if (a.price == b.price) {
            return a.timestamp < b.timestamp; // earlier orders have priority
        }
        return a.price < b.price;
    });

    // match while there is an overlap between highest bid and lowest ask
    while (!bidOrders.empty() && !askOrders.empty() && bidOrders.front().price >= askOrders.front().price) {
        Order& buy = bidOrders.front();
        Order& sell = askOrders.front();

        // determine trade quantity (fill as much as possible)
        int tradeQuantity = std::min(buy.quantity, sell.quantity);
        
        // determine trade price (using the resting order price)
        int tradePrice;

        if (buy.timestamp > sell.timestamp) {
            // If the buy order is newer, use the sell price
            tradePrice = sell.price;
        } else {
            // If the sell order is newer, use the buy price
            tradePrice = buy.price;
        }

        std::cout << "Matched BUY id " << buy.id
             << " (price " << buy.price << ", qty " << tradeQuantity << ") with SELL id " << sell.id
             << " (price " << sell.price << ", qty " << tradeQuantity << ") at trade price " << tradePrice << std::endl;

        // deduct executed quantity from both orders
        buy.quantity -= tradeQuantity;
        sell.quantity -= tradeQuantity;

        // callback hook; notify execution for BOTH resting orders crossing
        if (listener_) {
            listener_->on_order_executed(buy, tradePrice, tradeQuantity);
            listener_->on_order_executed(sell, tradePrice, tradeQuantity);
        }

        // remove fully filled orders using pop_front() for deques
        if (buy.quantity > 0) {
            buy.status = OrderStatus::PARTIALLY_FILLED;
        } else {
            buy.status = OrderStatus::FILLED;
            bidOrders.pop_front();
        }
        if (sell.quantity > 0) {
            sell.status = OrderStatus::PARTIALLY_FILLED;
        } else {
            sell.status = OrderStatus::FILLED;
            askOrders.pop_front();
        }
    }
}

std::optional<Order> OrderBook::cancelOrderAndReturn(int orderId) {
    // Search for the order in ask orders
    auto askIt = std::find_if(askOrders.begin(), askOrders.end(), [orderId](const Order& o) {
        return o.id == orderId || o.clOrdID == orderId;
    });

    if (askIt != askOrders.end()) {
        Order cancelledOrder = *askIt;
        cancelledOrder.status = OrderStatus::CANCELLED;
        std::cout << "[OrderBook] Cancelled SELL order ID: " << askIt->id 
                  << " (ClOrdID: " << askIt->clOrdID << ")" << std::endl;
        askOrders.erase(askIt);
        return cancelledOrder;
    }

    // Search for the order in bid orders
    auto bidIt = std::find_if(bidOrders.begin(), bidOrders.end(), [orderId](const Order& o) {
        return o.id == orderId || o.clOrdID == orderId;
    });

    if (bidIt != bidOrders.end()) {
        Order cancelledOrder = *bidIt;
        cancelledOrder.status = OrderStatus::CANCELLED;
        std::cout << "[OrderBook] Cancelled BUY order ID: " << bidIt->id 
                  << " (ClOrdID: " << bidIt->clOrdID << ")" << std::endl;
        bidOrders.erase(bidIt);
        return cancelledOrder;
    }

    std::cout << "[OrderBook] Order ID / ClOrdID " << orderId << " not found for cancellation." << std::endl;
    return std::nullopt;
}

void OrderBook::level1Data() const {
    if (!bidOrders.empty()) {
        std::cout << "Best Bid: $" << bidOrders.front().price << " x " << bidOrders.front().quantity << std::endl;
    } else {
        std::cout << "Best Bid: N/A" << std::endl;
    }

    if (!askOrders.empty()) {
        std::cout << "Best Ask: $" << askOrders.front().price << " x " << askOrders.front().quantity << std::endl;
    } else {
        std::cout << "Best Ask: N/A" << std::endl;
    }
}

void OrderBook::level2Data() const {
    // 1. Aggregate Bids (Highest Price first)
    std::map<int, int, std::greater<int>> aggregatedBids;
    for (const auto& order : bidOrders) {
        aggregatedBids[order.price] += order.quantity;
    }

    // 2. Aggregate Asks (Lowest Price first)
    std::map<int, int, std::less<int>> aggregatedAsks;
    for (const auto& order : askOrders) {
        aggregatedAsks[order.price] += order.quantity;
    }

    // 3. Render a beautiful Side-by-Side Market Depth Dashboard
    std::cout << "\n================= LEVEL 2 MARKET DEPTH =================" << std::endl;
    std::cout << std::left << std::setw(15) << "BID QUANTITY" << std::setw(12) << "BID PRICE" 
         << " | " 
         << std::left << std::setw(12) << "ASK PRICE" << std::setw(15) << "ASK QUANTITY" << std::endl;
    std::cout << "--------------------------------------------------------" << std::endl;

    auto bidIt = aggregatedBids.begin();
    auto askIt = aggregatedAsks.begin();

    // Loop until both books run out of price depth layers
    while (bidIt != aggregatedBids.end() || askIt != aggregatedAsks.end()) {
        // Print Bid Column
        if (bidIt != aggregatedBids.end()) {
            std::cout << std::left << std::setw(15) << bidIt->second 
                      << "$" << std::setw(11) << bidIt->first;
            ++bidIt;
        } else {
            std::cout << std::left << std::setw(15) << "" << std::setw(12) << ""; // Blank pad
        }

        std::cout << " | ";

        // Print Ask Column
        if (askIt != aggregatedAsks.end()) {
            std::cout << "$" << std::left << std::setw(11) << askIt->first 
                 << std::setw(15) << askIt->second;
            ++askIt;
        } else {
            std::cout << std::left << std::setw(12) << "" << std::setw(15) << ""; // Blank pad
        }
        std::cout << std::endl;
    }
    std::cout << "========================================================" << std::endl;
}

void OrderBook::set_listener(IExecutionListener* listener) {
    listener_ = listener;
}

void OrderBook::remove_listener() {
    listener_ = nullptr;
}

int OrderBook::getNextOrderId() {
    return nextOrderId++;
}