#include <gtest/gtest.h>
#include "gateways/FIXGateway.hpp"
#include "writers/FIXWriter.hpp"
#include "parsers/FIXParser.hpp"
#include "gateways/FIXDefinition.hpp"
#include "orderbook/Order.hpp"
#include <string>

TEST(FIXWriterTest, WriteOrder) {
    FIXWriter writer;
    Order order{OrderType::LIMIT, OrderDirection::BUY, 100, 10, 10, 1234567890, 1, OrderStatus::NEW, 0};
    char buffer[1024];
    size_t bytesWritten = writer.write(order, buffer, sizeof(buffer));

    EXPECT_GT(bytesWritten, 0);
    std::string output(buffer, bytesWritten);

    EXPECT_NE(output.find("35=D"), std::string::npos);
    EXPECT_NE(output.find("54=1"), std::string::npos);
    EXPECT_NE(output.find("38=10"), std::string::npos);  // Corrected: quantity is 10
    EXPECT_NE(output.find("44=100"), std::string::npos); // Corrected: price is 100
    EXPECT_NE(output.find("11=1"), std::string::npos);
}

TEST(FIXParserTest, ParseOrder) {
    FIXParser parser;
    // Provide an integer timestamp field (Tag 60) so validation doesn't reject it for being 0 (with a value of 1685622600000)
    std::string fixMessage = "8=FIX.4.2|9=176|35=D|49=CLIENT1|56=SERVER1|34=1|11=12345|21=1|55=AAPL|54=1|38=100|40=2|44=150|60=1685622600000|10=128|";

    // Replace '|' with SOH character for actual parsing
    for (char& c : fixMessage) {
        if (c == '|') c = '\x01';
    }

    auto orderOpt = parser.parse(fixMessage);

    ASSERT_TRUE(orderOpt.has_value());
    Order order = orderOpt.value();

    EXPECT_EQ(order.type, OrderType::LIMIT);
    EXPECT_EQ(order.direction, OrderDirection::BUY);
    EXPECT_EQ(order.price, 150);
    EXPECT_EQ(order.quantity, 100);
}

TEST(FIXParserBoundaryTest, SplitsTwoConcatenatedMessagesCorrectly) {
    const char soh = '\x01';
    std::string msg1 = std::string("8=FIX.4.2") + soh + "9=61" + soh + "35=D" + soh + "44=20" + soh + "38=4" + soh + "11=8002" + soh + "60=181433688181400" + soh + "40=2" + soh + "54=2" + soh + "55=AAPL" + soh + "10=085" + soh;
    std::string msg2 = std::string("8=FIX.4.2") + soh + "9=61" + soh + "35=D" + soh + "44=20" + soh + "38=4" + soh + "11=8002" + soh + "60=181433688181400" + soh + "40=2" + soh + "54=2" + soh + "55=NVDA" + soh + "10=096" + soh;
    std::string combined = msg1 + msg2;

    size_t firstLen = FIXParser::find_message_boundary(combined);
    ASSERT_EQ(firstLen, msg1.size());

    std::string_view remaining(combined.data() + firstLen, combined.size() - firstLen);
    size_t secondLen = FIXParser::find_message_boundary(remaining);
    ASSERT_EQ(secondLen, msg2.size());

    // Confirm each slice parses independently and doesn't leak fields across the boundary
    auto order1 = FIXParser().parse(std::string_view(combined.data(), firstLen));
    auto order2 = FIXParser().parse(std::string_view(combined.data() + firstLen, secondLen));

    ASSERT_TRUE(order1.has_value());
    ASSERT_TRUE(order2.has_value());
    EXPECT_EQ(std::string_view(order1->symbol.data(), 4), "AAPL");
    EXPECT_EQ(std::string_view(order2->symbol.data(), 4), "NVDA");
}

TEST(FIXParserTest, ParseLogonHeader) {
    FIXParser parser;
    const char soh = '\x01';
    std::string fixMessage = std::string("8=FIX.4.2") + soh + "9=45" + soh + "35=A" + soh + "49=1001" + soh + "56=EXCHANGE" + soh + "10=100" + soh;
    
    ParsedFIXHeader header = parser.parse_header(fixMessage);
    
    // Ensure MsgType 'A' (Logon) is correctly identified
    EXPECT_EQ(header.msgType, FIX::MsgTypes::Logon);
    
    // Ensure SenderCompID is extracted natively as an integer
    EXPECT_EQ(header.senderCompID, 1001);
}

TEST(FIXParserTest, ParseOrderCancelHeader) {
    FIXParser parser;
    const char soh = '\x01';
    std::string fixMessage = std::string("8=FIX.4.2") + soh + "9=55" + soh + "35=F" + soh + "49=8002" + soh + "11=1003" + soh + "41=1002" + soh + "55=NVDA" + soh + "10=080" + soh;
    
    ParsedFIXHeader header = parser.parse_header(fixMessage);
    
    // Ensure MsgType 'F' (Order Cancel Request) is correctly identified
    EXPECT_EQ(header.msgType, FIX::MsgTypes::OrderCancelRequest);
    
    // Ensure OrigClOrdID and SenderCompID are extracted
    EXPECT_EQ(header.origClOrdID, 1002);
    EXPECT_EQ(header.senderCompID, 8002);
}

TEST(FIXParserTest, ParseHeaderIgnoresIrrelevantTags) {
    FIXParser parser;
    const char soh = '\x01';
    // Standard NewOrderSingle, header parser should only pull MsgType and SenderCompID
    std::string fixMessage = std::string("8=FIX.4.2") + soh + "9=60" + soh + "35=D" + soh + "49=9999" + soh + "11=1001" + soh + "54=2" + soh + "40=2" + soh + "44=20" + soh + "38=10" + soh + "10=032" + soh;

    ParsedFIXHeader header = parser.parse_header(fixMessage);
    
    EXPECT_EQ(header.msgType, FIX::MsgTypes::NewOrderSingle);
    EXPECT_EQ(header.senderCompID, 9999);
    // OrigClOrdID should remain its default uninitialized value (e.g., -1 or 0 depending on your struct)
    EXPECT_EQ(header.origClOrdID, -1); 
}

TEST(FIXWriterTest, WriteLogonAcknowledgment) {
    FIXWriter writer;
    char buffer[256];
    int testClientID = 4040;
    
    size_t bytesWritten = writer.writeLogonAcknowledgment(testClientID, buffer, sizeof(buffer));
    
    EXPECT_GT(bytesWritten, 0);
    EXPECT_LT(bytesWritten, sizeof(buffer));
    
    std::string output(buffer, bytesWritten);
    const char soh = '\x01';
    
    EXPECT_NE(output.find(std::string("35=A") + soh), std::string::npos);
    EXPECT_NE(output.find(std::string("49=4040") + soh), std::string::npos); // SenderCompID
    EXPECT_NE(output.find(std::string("56=0") + soh), std::string::npos);    // TargetCompID
    EXPECT_NE(output.find("10="), std::string::npos);
}