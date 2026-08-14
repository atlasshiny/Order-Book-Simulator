#include "gateways/FIXGateway.hpp"
#include "parsers/FIXParser.hpp"
#include "writers/FIXWriter.hpp"
#include "gateways/FIXDefinition.hpp"
#include "network/TCPSession.hpp"
#include "orchestrator/ExchangeOrchestrator.hpp"
#include <iostream>
#include <string>

// Serialize and send an order to the exchange (assumes the order is already built and validated)
size_t FIXGateway::sendOrder(const Order& order, char* wireBuffer_, size_t bufferSize) {
    // Serialize the order into FIX format
    size_t bytesWritten = fixWriter_.write(order, wireBuffer_, bufferSize);
        
    if (bytesWritten == 0) {
        std::cerr << "Error: FIXWriter failed to serialize the message (buffer too small)." << std::endl;
        return 0;
    }

    // Here you would typically send the wireBuffer_ over a network socket
    std::cout << "Sending FIX message: ";
    for (size_t i = 0; i < bytesWritten; ++i) {
        if (wireBuffer_[i] == '\x01') std::cout << '|';
        else std::cout << wireBuffer_[i];
    }
    std::cout << std::endl;

    return bytesWritten; // Return the number of bytes written for confirmation
}

// Receive and parse an incoming FIX message from the exchange
std::optional<Order> FIXGateway::receiveOrder(std::string_view rawData) {
    // Parse the incoming FIX message
    auto orderOpt = fixParser_.parse(rawData);
        
    if (!orderOpt) {
        std::cerr << "Error: FIXParser failed to parse the incoming message." << std::endl;
        return std::nullopt;
    }

    const Order& order = *orderOpt;
    // Here you would typically process the order (e.g., add it to the order book)
    std::cout << "Received Order - Type: " << (order.type == OrderType::LIMIT ? "LIMIT" : "MARKET")
                << ", Symbol: " << std::string(order.symbol.data(), order.symbol.size())
                << ", Direction: " << (order.direction == OrderDirection::BUY ? "BUY" : "SELL")
                << ", Price: " << order.price
                << ", Quantity: " << order.quantity
                << ", Timestamp: " << order.timestamp
                << ", Status: " << (order.status == OrderStatus::NEW ? "NEW" : "CLOSED")
                << ", ClientID: " << order.clientID
                << std::endl;
    return orderOpt;
}

std::optional<Order> FIXGateway::on_data_received(std::shared_ptr<TCPSession> session, std::string_view raw_data) {
    // Inspect FIX Header
    ParsedFIXHeader header = fixParser_.parse_header(raw_data);

    // Handle Logon (35=A)
    if (header.msgType == FIX::MsgTypes::Logon) {
        session->set_authenticated(true);
        session->set_clientID(header.senderCompID);

        std::cout << "[FIXGateway] Client " << header.senderCompID << " authenticated via Logon (35=A).\n";

        char buffer[256];
        size_t len = fixWriter_.writeLogonAcknowledgment(header.senderCompID, buffer, sizeof(buffer));
        session->write(std::string(buffer, len));

        return std::nullopt; // Swallowed internally!
    }

    // Reject Unauthenticated Messages
    if (!session->is_authenticated()) {
        std::cout << "[FIXGateway] Rejecting unauthenticated message.\n";
        return std::nullopt;
    }

    // Handle Order Cancel (35=F)
    if (header.msgType == FIX::MsgTypes::OrderCancelRequest) {
        std::optional<Order> order = fixParser_.parse(raw_data);
        if (order) {
            order->clientID = session->get_clientID();
            // Override the clOrdID with the target ID we want to cancel
            order->clOrdID = header.origClOrdID; 
        }
        return order; 
    }

    // Handle New Order Single (35=D)
    if (header.msgType == FIX::MsgTypes::NewOrderSingle) {
        std::optional<Order> order = fixParser_.parse(raw_data);
        if (order) {
            order->clientID = session->get_clientID(); 
        }
        return order; 
    }

    return std::nullopt;
}

void FIXGateway::on_client_connect(std::shared_ptr<TCPSession> session) {
    std::cout << "New client connected " << std::endl;
}

void FIXGateway::on_client_disconnect(std::shared_ptr<TCPSession> session) {
    std::cout << "A client disconnected " << std::endl;
}

FIXWriter& FIXGateway::get_writer() {
    return fixWriter_;
}

void FIXGateway::set_orchestrator(std::shared_ptr<ExchangeOrchestrator> orchestrator) {
    orchestrator_ = orchestrator;
}