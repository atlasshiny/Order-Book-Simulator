#pragma once
#include "parsers/IParser.hpp"

class FIXParser : public IParser {
    public:
        std::optional<Order> parse(std::string_view rawData) override;

        // checks buffer for complete FIX message and returns the length of the message or 0 if incomplete
        static size_t find_message_boundary(std::string_view buffer);
};