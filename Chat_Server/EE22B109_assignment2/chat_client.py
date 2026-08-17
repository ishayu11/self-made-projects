import socket
import json
import threading
import getpass

# Hotspot IP from your configuration
SERVER_IP = '172.20.10.4'


def listen_for_messages(sock):
    while True:
        try:
            data = sock.recv(2048).decode()
            if not data: break
            msg = json.loads(data)

            if msg["type"] == "MSG":
                print(f"\n[{msg['mode']}] {msg['from']}: {msg['content']}\n> ", end="")
            elif msg["type"] == "DATA":
                print(f"\n[SYSTEM] Online: {', '.join(msg['content'])}\n> ", end="")
            elif msg["type"] == "HISTORY_DATA":  # Bonus: Render history
                print("\n--- CHAT HISTORY ---")
                for log in msg["content"]:
                    print(f"[{log['timestamp']}] {log['from']} -> {log['to']}: {log['content']}")
                print("--------------------\n> ", end="")
        except:
            break


def start_client():
    user = input("Username: ")
    pwd = getpass.getpass("Password: ")

    try:
        # Step 1: Registration
        disc = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        disc.settimeout(5.0)
        disc.connect((SERVER_IP, 5000))
        disc.send(json.dumps({"type": "REGISTER", "username": user, "password": pwd,
                              "details": {"ip": SERVER_IP, "port": 8081}}).encode())
        disc.close()

        # Step 2: Chat Authentication
        sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        sock.connect((SERVER_IP, 8080))
        sock.send(json.dumps({"type": "AUTH", "username": user, "password": pwd}).encode())

        resp = json.loads(sock.recv(1024).decode())
        if resp.get("content") == "SUCCESS":
            print(f"Logged in. Commands: /list, /status <type>, /history, @name <msg>, or 'quit'.")
            threading.Thread(target=listen_for_messages, args=(sock,), daemon=True).start()

            while True:
                text = input("> ")
                if text.lower() == 'quit':
                    sock.send(json.dumps({"type": "EXIT"}).encode())
                    break
                elif text.startswith("/status"):  # Bonus
                    parts = text.split(" ", 1)
                    if len(parts) > 1:
                        sock.send(json.dumps({"type": "STATUS_UPDATE", "status": parts[1]}).encode())
                elif text.startswith("/history"):  # Bonus
                    sock.send(json.dumps({"type": "GET_HISTORY"}).encode())
                elif text.startswith("/list"):
                    sock.send(json.dumps({"type": "LIST"}).encode())
                elif text.startswith("@"):
                    parts = text.split(" ", 1)
                    if len(parts) > 1:
                        sock.send(json.dumps({"type": "PRIV", "target": parts[0][1:], "content": parts[1]}).encode())
                else:
                    sock.send(json.dumps({"type": "BCAST", "content": text}).encode())
        else:
            print("Login failed.")
    except Exception as e:
        print(f"Error: {e}")


if __name__ == "__main__":
    start_client()