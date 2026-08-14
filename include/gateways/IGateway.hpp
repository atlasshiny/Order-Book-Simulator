#pragma once
#include "orderbook/Order.hpp"
#include "writers/IWriter.hpp"
#include "parsers/IParser.hpp"
#include <optional>
#include <string_view>
#include <memory>

class TCPSession; // Forward declaration

class IGateway {
    public:
        virtual ~IGateway() = default;

        // Called by TCPSession when raw bytes arrive over the wire
        virtual std::optional<Order> on_data_received(std::shared_ptr<TCPSession> session, std::string_view raw_data) = 0;

        // Called by TCPSession when a connection drops or initiates
        virtual void on_client_connect(std::shared_ptr<TCPSession> session) = 0;
        virtual void on_client_disconnect(std::shared_ptr<TCPSession> session) = 0;

        virtual IWriter& get_writer() = 0;
        virtual IParser& get_parser() = 0;

    private:
        virtual size_t sendOrder(const Order& order, char* wireBuffer_, size_t bufferSize) = 0;
        virtual std::optional<Order> receiveOrder(std::string_view rawData) = 0;
};