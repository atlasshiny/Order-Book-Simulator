#include "network/TCPSession.hpp"
#include <asio.hpp>
#include <iostream>

TCPSession::TCPSession(asio::ip::tcp::socket socket, std::shared_ptr<ExchangeOrchestrator> orchestrator)
    : socket_(std::move(socket)), orchestrator_(std::move(orchestrator)) {}

void TCPSession::start() {
    // Notify the orchestrator that a new client has connected
    if (orchestrator_) {
        orchestrator_->on_client_connect(shared_from_this());
    }

    // Start reading data from the client
    do_read();
}

void TCPSession::write(const std::string& data) {
    bool write_in_progress = !pending_writes_.empty();
    pending_writes_.push_back(data);
    if (!write_in_progress) {
        do_write();
    }
}

void TCPSession::close() {
    std::error_code ec;
    socket_.close(ec);

    // Notify the orchestrator that the client has disconnected
    if (orchestrator_) {
        orchestrator_->on_client_disconnect(shared_from_this());
    }
}

void TCPSession::do_read() {
    auto self(shared_from_this()); // Keep the session alive during the async operation

    socket_.async_read_some(asio::buffer(read_buffer_),
        [this, self](std::error_code ec, std::size_t length) {
            if (!ec) {
                // Process the received data into a string_view and pass it to the orchestrator
                std::string_view raw_data(read_buffer_.data(), length);
                if (orchestrator_) {
                    orchestrator_->on_data_received(self, raw_data);
                }

                // Continue reading data
                do_read();
            } else {
                // Handle error or disconnection
                close();
            }
        });
}

void TCPSession::do_write() {
    auto self(shared_from_this()); // Keep the session alive during the async operation

    asio::async_write(socket_, asio::buffer(pending_writes_.front()),
        [this, self](std::error_code ec, std::size_t /*length*/) {
            if (!ec) {
                pending_writes_.erase(pending_writes_.begin());

                // If there are more messages waiting in the queue, write the next one
                if (!pending_writes_.empty()) {
                    do_write();
                }
            } else {
                // Handle error or disconnection
                close();
            }
        });

}


char* TCPSession::get_read_buffer_ptr() {
    return read_buffer_.data();
}

size_t TCPSession::get_read_buffer_size() {
    return read_buffer_.size();
}

char* TCPSession::get_write_buffer_ptr() {
    return write_buffer_.data();
}

size_t TCPSession::get_write_buffer_size() {
    return write_buffer_.size();
}