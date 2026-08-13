#include "parsers/FIXParser.hpp"
#include "gateways/FIXDefinition.hpp"
#include <charconv>

std::optional<Order> FIXParser::parse(std::string_view rawData) {
    Order order{};
    size_t pos = 0;

    // Complete basic message checks
    if (rawData.empty()) {
        return std::nullopt; // No data to parse
    }

    // Safely trim leading whitespace
    size_t first_not_ws = rawData.find_first_not_of(" \t\n\r\f\v");
    if (first_not_ws != std::string_view::npos) {
        rawData.remove_prefix(first_not_ws);
    } else {
        return std::nullopt; // String is entirely whitespace
    }

    // Safely trim trailing whitespace
    size_t last_not_ws = rawData.find_last_not_of(" \t\n\r\f\v");
    if (last_not_ws != std::string_view::npos) {
        rawData.remove_suffix(rawData.size() - (last_not_ws + 1));
    }

    while (pos < rawData.size()) {
        // Process each chunk of data separated by the SOH delimiter
        std::string_view chunk = rawData.substr(pos);
        size_t nextPos = chunk.find_first_of('\x01'); // SOH delimiter in FIX messages
        if (nextPos == std::string_view::npos) {
            break; // No more chunks to process
        }
        chunk = chunk.substr(0, nextPos); // Get the current chunk
        pos += nextPos + 1; // Move to the next chunk

        // Skip empty chunks caused by the trailing delimiter
        if (chunk.empty()) {
            continue; 
        }

        size_t eqPos = chunk.find('='); // Find the position of the '=' character
        if (eqPos == std::string_view::npos) {
            return std::nullopt; // malformed field
        }

        std::string_view tagStr = chunk.substr(0, eqPos); // Extract the tag number
        std::string_view valStr = chunk.substr(eqPos + 1); // Extract the value

        int tag = 0;
        // Convert the tag string to an int to match your FIX::Tags definitions
        auto result = std::from_chars(tagStr.data(), tagStr.data() + tagStr.size(), tag);
        
        if (result.ec == std::errc()) {
            switch(tag) {
                case FIX::Tags::MsgType: {
                    if (valStr == "D") {
                        order.type = OrderType::LIMIT; // Assuming NewOrderSingle corresponds to a LIMIT order
                    } else {
                        return std::nullopt; // Unsupported message type
                    }
                    break;
                }

                case FIX::Tags::PRICE: {
                    // Use from_chars for the double/int value
                    int parsedPrice = 0;
                    auto res = std::from_chars(valStr.data(), valStr.data() + valStr.size(), parsedPrice);
                    
                    if (res.ec == std::errc()) {
                        order.price = parsedPrice;
                    }
                    break;
                }

                case FIX::Tags::QUANTITY: {
                    // Use from_chars for the quantity
                    int parsedQuantity = 0;
                    auto res = std::from_chars(valStr.data(), valStr.data() + valStr.size(), parsedQuantity);
                    
                    if (res.ec == std::errc()) {
                        order.quantity = parsedQuantity;
                    }
                    break;
                }

                case FIX::Tags::OrdType: {
                    if (valStr == "1") {
                        order.type = OrderType::MARKET;
                    } else if (valStr == "2") {
                        order.type = OrderType::LIMIT;
                    }
                    break;
                }

                case FIX::Tags::SIDE: {
                    if (valStr == "1") {
                        order.direction = OrderDirection::BUY;
                    } else if (valStr == "2") {
                        order.direction = OrderDirection::SELL;
                    }
                    break;
                }

                case FIX::Tags::OrderID: {
                    // Use from_chars for the OrderID
                    int parsedOrderID = 0;
                    auto res = std::from_chars(valStr.data(), valStr.data() + valStr.size(), parsedOrderID);
                    
                    if (res.ec == std::errc()) {
                        order.id = parsedOrderID;
                    }
                    break;
                }

                case FIX::Tags::TIMESTAMP: {
                    // Use from_chars for the timestamp
                    uint64_t parsedTimestamp = 0;
                    auto res = std::from_chars(valStr.data(), valStr.data() + valStr.size(), parsedTimestamp);
                    
                    if (res.ec == std::errc()) {
                        order.timestamp = parsedTimestamp;
                    }
                    break;
                }

                case FIX::Tags::ClOrdID: {
                    // Use from_chars for the client ID
                    int parsedClientID = 0;
                    auto res = std::from_chars(valStr.data(), valStr.data() + valStr.size(), parsedClientID);
                    
                    if (res.ec == std::errc()) {
                        order.clientID = parsedClientID;
                    }
                    break;
                }

                case FIX::Tags::Symbol: {
                    // Safely copy the symbol into the fixed-size array
                    size_t copyLen = std::min(valStr.size(), order.symbol.size());
                    std::memcpy(order.symbol.data(), valStr.data(), copyLen);
                    if (copyLen < order.symbol.size()) {
                        order.symbol[copyLen] = '\0'; // Null-terminate if there's space
                    }
                    break;
                }

                default:
                    // Safely ignore Tag 8, Tag 9, Tag 10, etc., without failing
                    break;
            }   
        }
    }

    if (order.type == OrderType::NONE || order.price <= 0 || order.quantity <= 0 || order.timestamp == 0) {
        return std::nullopt; // The message was incomplete or invalid
    }

    return order;
}

ParsedFIXHeader FIXParser::parse_header(std::string_view rawData) {
    ParsedFIXHeader header;

    // Search through SOH (\x01) delimited fields
    size_t start = 0;
    while (start < rawData.size()) {
        size_t end = rawData.find('\x01', start);

        std::string_view field = rawData.substr(start, end - start);
        size_t eqPos = field.find('=');

        if (eqPos != std::string_view::npos) {
            std::string_view tagStr = field.substr(0, eqPos);
            std::string_view valStr = field.substr(eqPos + 1);

            int tag = 0;
            auto [ptr, ec] = std::from_chars(tagStr.data(), tagStr.data() + tagStr.size(), tag);

            if (ec == std::errc()) {
                switch (tag) {
                    case FIX::Tags::MsgType:
                        if (!valStr.empty()) {
                            header.msgType = valStr[0];
                        }
                        break;

                    case FIX::Tags::SenderCompID:
                        std::from_chars(valStr.data(), valStr.data() + valStr.size(), header.senderCompID);
                        break;

                    case FIX::Tags::OrigClOrdID:
                        std::from_chars(valStr.data(), valStr.data() + valStr.size(), header.origClOrdID);
                        break;

                    default:
                        // Ignore unneeded tags
                        break;
                }
            }
        }

        start = end + 1;
    }

    return header;
}

size_t FIXParser::find_message_boundary(std::string_view buffer) {
    // Check for the presence of the SOH delimiter
    size_t sohPos = buffer.find('\x01');
    if (sohPos == std::string_view::npos) {
        return 0; // No complete message found
    }

    // Check for the presence of the checksum field (Tag 10)
    size_t checksumPos = buffer.find(std::to_string(FIX::Tags::Checksum) + "=", sohPos);
    if (checksumPos == std::string_view::npos) {
        return 0; // No complete message found
    }

    // Find the end of the checksum field
    size_t endOfChecksum = buffer.find('\x01', checksumPos);
    if (endOfChecksum == std::string_view::npos) {
        return 0; // No complete message found
    }

    // Return the length of the complete message including the checksum field
    return endOfChecksum + 1; // +1 to include the SOH delimiter at the end
}