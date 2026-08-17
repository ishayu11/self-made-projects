import socket
import threading
import json

# In-memory database to store registered users
# Format: { "username": {"password": "pwd"} }
database = {}


def handle_client(conn, addr):
    """
    Handles registration and querying of users.
    Fixes the 'details' key error by properly parsing performance test requests.
    """
    try:
        data = conn.recv(1024).decode()
        if not data:
            return

        req = json.loads(data)
        r_type = req.get("type")

        # 1. Registration Logic (Used by Performance Test & Clients)
        if r_type == "REGISTER":
            user = req.get("username")
            pwd = req.get("password")

            if user and pwd:
                database[user] = {"password": pwd}
                conn.send(json.dumps("SUCCESS").encode())
                print(f"[REG] Registered: {user} from {addr[0]}")
            else:
                conn.send(json.dumps("FAILED: Missing Fields").encode())

        # 2. Query Logic (Used by Chat Server for Authentication)
        elif r_type == "QUERY":
            target = req.get("target")
            if target in database:
                # Return the stored credentials for verification
                conn.send(json.dumps(database[target]).encode())
            else:
                conn.send(json.dumps("NOT_FOUND").encode())

    except Exception as e:
        print(f"[ERR] Discovery Server Error: {e}")
    finally:
        conn.close()


def start_discovery():
    """
    Starts the Discovery Server on Port 5000.
    """
    server = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    server.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)

    # Bind to 0.0.0.0 to allow connections from the hotspot IP
    server.bind(('0.0.0.0', 5000))
    server.listen(20)

    print("Discovery Server active on port 5000...")
    print("Waiting for registration/query requests...")

    while True:
        conn, addr = server.accept()
        threading.Thread(target=handle_client, args=(conn, addr)).start()


if __name__ == "__main__":
    start_discovery()