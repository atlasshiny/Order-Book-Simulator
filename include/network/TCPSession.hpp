#pragma once
#include <asio.hpp>
#include <memory>
#include "orchestrator/ExchangeOrchestrator.hpp"

// TCPSession class that manages a single TCP connection
class TCPSession : public std::enable_shared_from_this<TCPSession> {
public:
    TCPSession(asio::ip::tcp::socket socket, std::shared_ptr<ExchangeOrchestrator> orchestrator);
    void start();
    void write(const std::string& data);
    void close();

    // get methods for the read and write buffer pointer and size
    char* get_read_buffer_ptr();
    size_t get_read_buffer_size();
    
    char* get_write_buffer_ptr();
    size_t get_write_buffer_size();

    // set and get method for the authentication status and clientID
    void set_authenticated(bool status) { authenticated_ = status; }
    bool is_authenticated() const { return authenticated_; }

    void set_clientID(int id) { clientID_ = id; }
    int get_clientID() const { return clientID_; }

private:
    void do_read();
    void do_write();

    asio::ip::tcp::socket socket_;
    std::array<char, 1024> read_buffer_;
    std::array<char, 1024> write_buffer_;

    std::array<char, 4096> buffer_; // Buffer to hold incoming data
    size_t buffer_len_ = 0; // Current length of data in the buffer

    std::vector<std::string> pending_writes_;
    std::shared_ptr<ExchangeOrchestrator> orchestrator_;

    bool authenticated_ = false; // Flag to indicate if the client is authenticated
    int clientID_ = -1; // Client ID associated with this session, initialized to -1 (invalid)
};