#include "network/TCPSession.hpp"
#include "parsers/FIXParser.hpp"
#include <asio.hpp>
#include <iostream>
#include <cstring>

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
                // Guard against overflowing the accumulator (malformed/malicious client
                // sending data with no valid message boundary)
                if (buffer_len_ + length > buffer_.size()) {
                    std::cerr << "[Session] Accumulator overflow - closing connection." << std::endl;
                    close();
                    return;
                }

                // Append the newly-read bytes onto whatever partial data is left over
                std::memcpy(buffer_.data() + buffer_len_, read_buffer_.data(), length);
                buffer_len_ += length;

                // Drain as many complete FIX messages as are currently available
                size_t consumed = 0;
                while (true) {
                    std::string_view accumView(buffer_.data() + consumed, buffer_len_ - consumed);
                    size_t msgLen = FIXParser::find_message_boundary(accumView);

                    if (msgLen == 0) {
                        break; // No complete message yet - wait for more bytes
                    }

                    std::string_view oneMessage(buffer_.data() + consumed, msgLen);
                    if (orchestrator_) {
                        orchestrator_->on_data_received(self, oneMessage);
                    }

                    consumed += msgLen;
                }

                // Shift only the leftover partial bytes down to the front
                if (consumed > 0) {
                    std::memmove(buffer_.data(), buffer_.data() + consumed, buffer_len_ - consumed);
                    buffer_len_ -= consumed;
                }

                // Continue reading more data
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
