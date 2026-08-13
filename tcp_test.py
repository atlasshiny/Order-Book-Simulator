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
    
    # Define our two test orders (Sell order rests, Buy order triggers a match)
    fix_message_sell_aapl = "8=FIX.4.2\x019=61\x0135=D\x0144=20\x0138=4\x0111=8002\x0160=181433688181400\x0140=2\x0154=2\x0155=AAPL\x0110=085\x01"
    fix_message_sell_nvda = "8=FIX.4.2\x019=61\x0135=D\x0144=20\x0138=4\x0111=8002\x0160=181433688181400\x0140=2\x0154=2\x0155=NVDA\x0110=096\x01"

    fix_message_buy_aapl  = "8=FIX.4.2\x019=60\x0135=D\x0111=123\x0154=1\x0140=1\x0144=20\x0138=3\x0160=181433688181401\x0155=AAPL\x0110=030\x01"
    fix_message_buy_nvda  = "8=FIX.4.2\x019=60\x0135=D\x0111=123\x0154=1\x0140=1\x0144=20\x0138=3\x0160=181433688181401\x0155=NVDA\x0110=041\x01"
    try:
        # Send the SELL order to build resting liquidity inside the book
        print("\nSending Mock FIX SELL Order (Price: 20, Qty: 4)...")
        s.sendall(fix_message_sell_aapl.encode('utf-8'))
        s.sendall(fix_message_sell_nvda.encode('utf-8'))

        # Immediately wait and listen for the SELL order confirmation (ExecType=0, OrdStatus=0)
        response = s.recv(1024)
        if response:
            print_fix_message("[SERVER RESPONSE - SELL ACCEPTANCE]:", response)

        # Small pause to separate events visually
        time.sleep(0.5)

        # Send the aggressive BUY order to cross the book at price 20
        print("\nSending Mock FIX BUY Order (Price: 20, Qty: 3)...")
        s.sendall(fix_message_buy_aapl.encode('utf-8'))
        s.sendall(fix_message_buy_nvda.encode('utf-8'))

        # Read responses
        s.settimeout(2.0) # Stop blocking if the server runs out of messages to send
        
        print("Listening for Execution Reports...")
        while True:
            response = s.recv(1024)
            if not response:
                break
            print_fix_message("[SERVER RESPONSE - MATCH ENGINE]:", response)

    except socket.timeout:
        print("\nFinished receiving messages (Socket timeout reached).")
    except Exception as e:
        print(f"An error occurred: {e}")
    finally:
        s.close()
        print("Connection closed safely.")

if __name__ == "__main__":
    main()