#pragma once
#include "parsers/IParser.hpp"

// Empty struct to hold parsed header information from a FIX message
struct ParsedFIXHeader {
    char msgType = '\0';
    int senderCompID = -1;
    int origClOrdID = -1;
    orderbook::Symbol symbol{};
};

class FIXParser : public IParser {
    public:
        std::optional<Order> parse(std::string_view rawData) override;

        // Helper to extract header details (MsgType, SenderCompID, OrigClOrdID)
        ParsedFIXHeader parse_header(std::string_view rawData);

        // checks buffer for complete FIX message and returns the length of the message or 0 if incomplete
        static size_t find_message_boundary(std::string_view buffer);
};