import subprocess, threading, psutil, socket, json, time, csv, os, argparse
from datetime import datetime


# --- Server Resource Monitoring Thread ---
class MonitorThread(threading.Thread):
    def __init__(self, server_pid, output_file, interval=1.0):
        super().__init__()
        self.server_pid = server_pid
        self.output_file = output_file
        self.interval = interval
        self.stop_event = threading.Event()
        self.process = psutil.Process(server_pid)

    def run(self):
        # Log headers for CPU and Memory
        with open(self.output_file, mode='w', newline='') as file:
            writer = csv.writer(file)
            writer.writerow(['timestamp', 'cpu_percent', 'memory_rss_mb'])

        while not self.stop_event.is_set():
            try:
                # Capture server metrics as required by Section 4
                cpu = self.process.cpu_percent(interval=0.1)
                rss = self.process.memory_info().rss / (1024 * 1024)
                with open(self.output_file, mode='a', newline='') as file:
                    csv.writer(file).writerow([datetime.now().strftime("%H:%M:%S"), cpu, rss])
            except psutil.NoSuchProcess:
                break
            self.stop_event.wait(self.interval)


# --- Client Latency Measurement Thread ---
class ClientThread(threading.Thread):
    def __init__(self, client_id, host, port, barrier, num_messages):
        super().__init__()
        self.client_id, self.host, self.port = client_id, host, port
        self.barrier, self.num_messages = barrier, num_messages
        self.metrics = []

    def run(self):
        try:
            # 1. Registration with Discovery
            reg = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            reg.connect((self.host, 5000))
            reg.send(json.dumps({"type": "REGISTER", "username": f"test_{self.client_id}", "password": "p"}).encode())
            reg.close()

            # 2. Connection to Chat Server
            sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            sock.connect((self.host, self.port))
            sock.send(json.dumps({"type": "AUTH", "username": f"test_{self.client_id}", "password": "p"}).encode())

            if "SUCCESS" in sock.recv(1024).decode():
                self.barrier.wait()  # Synchronized start for Load Test
                for _ in range(self.num_messages):
                    start = time.time()
                    sock.send(json.dumps({"type": "BCAST", "content": "perf_burst"}).encode())
                    sock.recv(1024)  # Wait for ACK
                    # Record message delivery time from send to receive
                    self.metrics.append(time.time() - start)
                    time.sleep(0.05)
            sock.close()
        except Exception as e:
            print(f"Client-{self.client_id} Error: {e}")


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--server-script', required=False, help='Path to chat_server.py')
    parser.add_argument('--server-mode', choices=['thread', 'fork', 'non-blocking'], default='non-blocking')
    parser.add_argument('--host', default='172.20.10.4')
    parser.add_argument('--num-messages', type=int, default=500)
    args = parser.parse_args()

    # --- SERVER MODE (Your Laptop) ---
    server_proc = None
    if args.server_script:
        print(f"[Main] Starting {args.server_mode} server and monitoring...")
        server_proc = subprocess.Popen(['python', args.server_script, '--mode', args.server_mode])
        monitor = MonitorThread(server_proc.pid, f"server_metrics_{args.server_mode}.csv")
        monitor.start()
        time.sleep(3)  # Warm up

    # --- CLIENT MODE (Friend's Laptop) ---
    print(f"[Main] Starting 10 concurrent clients for Load Test...")
    barrier = threading.Barrier(10)
    threads = [ClientThread(i, args.host, 8080, barrier, args.num_messages) for i in range(10)]

    for t in threads: t.start()
    for t in threads: t.join()

    # Log client metrics to file
    with open(f"client_metrics_{args.server_mode}.csv", "w", newline="") as f:
        writer = csv.writer(f);
        writer.writerow(['latency_ms'])
        for t in threads:
            for m in t.metrics: writer.writerow([m * 1000])

    if server_proc:
        monitor.stop_event.set()
        server_proc.terminate()
    print(f"[Main] Performance Benchmarking for {args.server_mode} complete.")


if __name__ == "__main__":
    main()