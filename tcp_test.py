import socket
import time
import select

def print_fix_message(prefix, raw_bytes):
    """Decodes raw FIX bytes and replaces the invisible SOH (\x01) delimiter with '|'"""
    decoded = raw_bytes.decode('utf-8', errors='ignore')
    readable_fix = decoded.replace('\x01', '|')
    print(f"{prefix} {readable_fix}")

def connect_and_logon(ip, port, client_id):
    """Helper to establish a socket and authenticate the session."""
    print(f"\n[Connecting] Client {client_id} to Exchange Server at {ip}:{port}...")
    s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    s.connect((ip, port))
    
    # Send Logon (35=A)
    logon_msg = f"8=FIX.4.2\x019=45\x0135=A\x0149={client_id}\x0156=EXCHANGE\x0110=100\x01"
    s.sendall(logon_msg.encode())
    
    # Read Logon ACK
    response = s.recv(1024)
    print_fix_message(f"[Client {client_id} LOGON ACK]:", response)
    return s

def main():
    server_ip = "127.0.0.1"
    server_port = 8080

    try:
        # Connect and Logon both clients via separate sockets
        client_a = connect_and_logon(server_ip, server_port, 1001)
        client_b = connect_and_logon(server_ip, server_port, 8002)

        # Client A (1001) places resting SELL orders
        print("\n[Step 2] Client 1001 seeding resting SELL orders (Price: $20, Qty: 10)...")
        # ClOrdID = 1001 (AAPL), ClOrdID = 1002 (NVDA)
        fix_sell_aapl = "8=FIX.4.2\x019=60\x0135=D\x0149=1001\x0111=1001\x0154=2\x0140=2\x0144=20\x0138=10\x0160=181433688181400\x0155=AAPL\x0110=032\x01"
        fix_sell_nvda = "8=FIX.4.2\x019=60\x0135=D\x0149=1001\x0111=1002\x0154=2\x0140=2\x0144=20\x0138=10\x0160=181433688181400\x0155=NVDA\x0110=043\x01"
        
        client_a.sendall(fix_sell_aapl.encode('utf-8'))
        client_a.sendall(fix_sell_nvda.encode('utf-8'))
        time.sleep(0.2)

        # Client A (1001) Cancels the NVDA Order
        print("\n[Step 3] Client 1001 canceling the NVDA order (OrigClOrdID = 1002)...")
        # MsgType = F, OrigClOrdID = 1002
        fix_cancel_nvda = "8=FIX.4.2\x019=55\x0135=F\x0149=1001\x0111=1003\x0141=1002\x0155=NVDA\x0110=080\x01"
        client_a.sendall(fix_cancel_nvda.encode('utf-8'))
        time.sleep(0.2)

        # Client B (8002) BUYs AAPL to Match & Fill
        print("\n[Step 4] Client 8002 sending BUY order to match AAPL (Price: $20, Qty: 10)...")
        fix_buy_aapl = "8=FIX.4.2\x019=60\x0135=D\x0149=8002\x0111=8001\x0154=1\x0140=2\x0144=20\x0138=10\x0160=181433688181401\x0155=AAPL\x0110=032\x01"
        client_b.sendall(fix_buy_aapl.encode('utf-8'))
        time.sleep(0.2)

        # Read all Execution Reports from both sockets
        print("\nListening for Server Responses / Execution Reports...")
        sockets = [client_a, client_b]
        
        # Read from sockets non-blockingly for up to 2 seconds
        end_time = time.time() + 2.0
        while time.time() < end_time:
            readable, _, _ = select.select(sockets, [], [], 0.5)
            for s in readable:
                response = s.recv(2048)
                if response:
                    owner = "Client 1001" if s == client_a else "Client 8002"
                    print_fix_message(f"[{owner} RECV]:", response)

    except Exception as e:
        print(f"An error occurred: {e}")
    finally:
        client_a.close()
        client_b.close()
        print("\nConnections closed safely.")

if __name__ == "__main__":
    main()