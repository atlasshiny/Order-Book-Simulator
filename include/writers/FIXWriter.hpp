#pragma once
#include "IWriter.hpp"

class FIXWriter : public IWriter {
    public:
        size_t write(const Order& order, char* buffer, size_t bufferSize) override;

        size_t writeExecutionReport(const Order& order, int lastShares, std::string_view execType, std::string_view ordStatus, char* buffer, size_t bufferSize) override;

        size_t writeLogonAcknowledgment(int clientID, char* buffer, size_t bufferSize);
};