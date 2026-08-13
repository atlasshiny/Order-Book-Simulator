import socket
import time

def print_fix_message(prefix, raw_bytes):
    """Decodes raw FIX bytes and replaces the invisible SOH (\x01) delimiter with '|'"""
    decoded = raw_bytes.decode('utf-8', errors='ignore')
    readable_fix = decoded.replace('\x01', '|')
    print(f"{prefix} {readable_fix}")

def main():
    server_ip = "127.0.0.1" #if the server is locally hosted
    server_port = 8080

    print(f"Connecting to C++ Exchange Server at {server_ip}:{server_port}...")
    s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    s.connect((server_ip, server_port))

    # STEP 1: Counterparty (Client 1001) places resting SELL orders at $20
    fix_counterparty_sell_aapl = "8=FIX.4.2\x019=60\x0135=D\x0111=1001\x0154=2\x0140=2\x0144=20\x0138=10\x0160=181433688181400\x0155=AAPL\x0110=032\x01"
    fix_counterparty_sell_nvda = "8=FIX.4.2\x019=60\x0135=D\x0111=1001\x0154=2\x0140=2\x0144=20\x0138=10\x0160=181433688181400\x0155=NVDA\x0110=043\x01"

    # STEP 2: Main Trader (Client 8002) BUYs at $20 to MATCH & FILL
    fix_trader_buy_aapl = "8=FIX.4.2\x019=60\x0135=D\x0111=8002\x0154=1\x0140=2\x0144=20\x0138=10\x0160=181433688181401\x0155=AAPL\x0110=032\x01"
    fix_trader_buy_nvda = "8=FIX.4.2\x019=60\x0135=D\x0111=8002\x0154=1\x0140=2\x0144=20\x0138=10\x0160=181433688181401\x0155=NVDA\x0110=043\x01"

    # STEP 3: Main Trader (Client 8002) SELLs 4 shares
    fix_trader_sell_aapl = "8=FIX.4.2\x019=61\x0135=D\x0111=8002\x0154=2\x0140=2\x0144=20\x0138=4\x0160=181433688181402\x0155=AAPL\x0110=087\x01"
    fix_trader_sell_nvda = "8=FIX.4.2\x019=61\x0135=D\x0111=8002\x0154=2\x0140=2\x0144=20\x0138=4\x0160=181433688181402\x0155=NVDA\x0110=098\x01"

    try:
        # Seed Liquidity
        print("\n[Step 1] Client 1001 seeding resting SELL orders (Price: $20, Qty: 10)...")
        s.sendall(fix_counterparty_sell_aapl.encode('utf-8'))
        s.sendall(fix_counterparty_sell_nvda.encode('utf-8'))
        time.sleep(0.2) # Allow C++ matching engine time to process resting asks

        # Match & Acquire Position
        print("\n[Step 2] Client 8002 sending BUY orders to match and fill (Price: $20, Qty: 10)...")
        s.sendall(fix_trader_buy_aapl.encode('utf-8'))
        s.sendall(fix_trader_buy_nvda.encode('utf-8'))
        time.sleep(0.2) # Allow C++ matching engine to execute trade and update portfolio

        # Sell Owned Shares
        print("\n[Step 3] Client 8002 selling part of acquired position (Qty: 4)...")
        s.sendall(fix_trader_sell_aapl.encode('utf-8'))
        s.sendall(fix_trader_sell_nvda.encode('utf-8'))

        # Read Execution Reports
        s.settimeout(2.0)
        print("\nListening for Server Responses / Execution Reports...")
        while True:
            response = s.recv(1024)
            if not response:
                break
            print_fix_message("[SERVER RESPONSE]:", response)

    except socket.timeout:
        print("\nFinished receiving messages (Socket timeout reached).")
    except Exception as e:
        print(f"An error occurred: {e}")
    finally:
        s.close()
        print("Connection closed safely.")

if __name__ == "__main__":
    main()