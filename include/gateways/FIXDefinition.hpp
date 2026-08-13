#pragma once

namespace FIX {
    // Standard FIX tags for order fields
    namespace Tags {
    // header tags for the messages
        constexpr int BeginString = 8;
        constexpr int BodyLength = 9;
        constexpr int MsgType = 35;
        constexpr int SenderCompID = 49;
        constexpr int TargetCompID = 56;
        constexpr int Checksum = 10;

        // common order fields
        constexpr int ClOrdID = 11;
        constexpr int OrigClOrdID = 41;
        constexpr int OrderID = 37;
        constexpr int OrdType = 40;
        constexpr int Symbol = 55;
        constexpr int SIDE = 54;
        constexpr int PRICE = 44;
        constexpr int QUANTITY = 38;
        constexpr int TIMESTAMP = 60;
        constexpr int ExecType = 150;
        constexpr int OrdStatus = 39;
        constexpr int LeavesQty = 151;
        constexpr int LastShares = 32;

        // Execution Report fields
        constexpr int ExecType = 150;
        constexpr int OrdStatus = 39;
        constexpr int LeavesQty = 151;
        constexpr int LastShares = 32;
        constexpr int Text = 58;
        // Add other message types as needed
    }

    namespace MsgTypes {
        constexpr char Logon = 'A';

        constexpr char NewOrderSingle = 'D';
        constexpr char ExecutionReport = '8';

        constexpr char OrderCancelRequest = 'F';
        constexpr char OrderCancelReplaceRequest = 'G';
        // Add other message types as needed
    }

    namespace ExecTypes {
        constexpr char New = '0';
        constexpr char PartialFill = '1';
        constexpr char Fill = '2';
        constexpr char DoneForDay = '3';
        constexpr char Canceled = '4';
        constexpr char Replaced = '5';
        constexpr char PendingCancel = '6';
        constexpr char Stopped = '7';
        constexpr char Rejected = '8';
        constexpr char Suspended = '9';
        constexpr char PendingNew = 'A';
        constexpr char Calculated = 'B';
        constexpr char Expired = 'C';
        constexpr char Trade = 'F';
        // Add other execution types as needed
    }

    namespace OrdStatuses {
        constexpr char New = '0';
        constexpr char PartiallyFilled = '1';
        constexpr char Filled = '2';
        constexpr char DoneForDay = '3';
        constexpr char Canceled = '4';
        constexpr char Replaced = '5';
        constexpr char PendingCancel = '6';
        constexpr char Stopped = '7';
        constexpr char Rejected = '8';
        constexpr char Suspended = '9';
        constexpr char PendingNew = 'A';
        constexpr char Calculated = 'B';
        constexpr char Expired = 'C';
        constexpr char Trade = 'F';
        // Add other order statuses as needed
    }
}