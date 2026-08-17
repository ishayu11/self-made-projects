import socket
import threading
import json
import time
import os
import queue

# --- Configuration & Global State ---
HISTORY_FILE = "chat_history.json"
clients = {}  # socket -> username
name_to_socket = {}  # username -> socket
user_statuses = {}  # username -> availability status

# --- Asynchronous Logging Module ---
history_queue = queue.Queue()


def logger_worker():
    """Sequential background writer to prevent file race conditions and I/O blocking."""
    while True:
        entry = history_queue.get()
        if entry is None: break

        history = []
        if os.path.exists(HISTORY_FILE):
            with open(HISTORY_FILE, "r") as f:
                try:
                    history = json.load(f)
                except:
                    history = []

        history.append(entry)
        with open(HISTORY_FILE, "w") as f:
            json.dump(history, f, indent=4)
        history_queue.task_done()


# Start the background logger thread
threading.Thread(target=logger_worker, daemon=True).start()


def save_to_history(sender, receiver, content, mode):
    """Adds message to memory queue; prevents the server from hanging on disk writes."""
    entry = {
        "timestamp": time.strftime("%Y-%m-%d %H:%M:%S"),
        "from": sender,
        "to": receiver,
        "mode": mode,
        "content": content
    }
    history_queue.put(entry)


# --- Business Logic & Protocol Handling ---

def verify_credentials(username, password):
    """Queries Discovery Server at localhost (assuming it runs on the same laptop)."""
    try:
        disc = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        disc.connect(('127.0.0.1', 5000))
        disc.send(json.dumps({"type": "QUERY", "target": username}).encode())
        response_data = disc.recv(2048).decode()
        response = json.loads(response_data)
        disc.close()
        return response != "NOT_FOUND" and response['password'] == password
    except:
        return False


def handle_client(conn, addr):
    username = None
    decoder = json.JSONDecoder()
    buffer = ""

    try:
        # 1. AUTH Handshake
        data = conn.recv(4096).decode()
        if not data: return
        msg = json.loads(data)

        if msg.get("type") == "AUTH" and verify_credentials(msg["username"], msg["password"]):
            username = msg["username"]
            conn.send(json.dumps({"type": "STATUS", "content": "SUCCESS"}).encode())
            clients[conn] = username
            name_to_socket[username] = conn
            user_statuses[username] = "available"
            print(f"[AUTH] {username} joined from {addr[0]}")
        else:
            conn.send(json.dumps({"type": "STATUS", "content": "FAILED"}).encode())
            conn.close()
            return

        # 2. Robust Communication Loop (Handles Sticky Packets)
        while True:
            data = conn.recv(4096).decode()
            if not data: break

            buffer += data
            # Process all complete JSON objects in the buffer
            while buffer.strip():
                try:
                    req, pos = decoder.raw_decode(buffer)
                    buffer = buffer[pos:].lstrip()

                    m_type = req.get("type")

                    if m_type == "BCAST":
                        save_to_history(username, "Global", req["content"], "Global")
                        payload = json.dumps(
                            {"type": "MSG", "from": username, "mode": "Global", "content": req["content"]})
                        for s in list(clients.keys()):
                            if s != conn:
                                try:
                                    s.send(payload.encode())
                                except:
                                    pass
                        # Send ACK for the performance test script to proceed
                        conn.send(json.dumps({"type": "ACK"}).encode())

                    elif m_type == "PRIV":
                        target = req.get("target")
                        if target in name_to_socket:
                            save_to_history(username, target, req["content"], "Private")
                            payload = json.dumps(
                                {"type": "MSG", "from": username, "mode": "Private", "content": req["content"]})
                            name_to_socket[target].send(payload.encode())
                        conn.send(json.dumps({"type": "ACK"}).encode())

                    elif m_type == "EXIT":
                        return

                except json.JSONDecodeError:
                    break  # Wait for more data to complete the JSON object

    except Exception as e:
        print(f"[ERR] {addr}: {e}")
    finally:
        if username and username in name_to_socket:
            del name_to_socket[username]
        if conn in clients:
            del clients[conn]
        conn.close()


def start_server():
    server = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    server.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    # Bind to 0.0.0.0 to listen for the client device
    server.bind(('0.0.0.0', 8080))
    server.listen(50)
    print("Chat Server listening on 0.0.0.0:8080 (Async Logging + Stream Decoding)...")
    while True:
        conn, addr = server.accept()
        threading.Thread(target=handle_client, args=(conn, addr)).start()


if __name__ == "__main__":
    start_server()