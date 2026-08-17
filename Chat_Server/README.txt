# Distributed Real-Time Chat System

## System Architecture Overview
The system follows a distributed client-server architecture consisting of three primary components:
1. **Discovery Server**: Acts as a central registry and authentication gatekeeper using a JSON-based protocol.
2. **Chat Server**: A multi-mode server supporting Threaded, Fork-based, and Non-blocking (Asynchronous) concurrency models. It features an **Asynchronous Logging Queue** to prevent disk I/O from blocking message relay.
3. **Chat Clients**: Individual nodes that connect via a shared network (Hotspot) to exchange global and private messages.

## Protocol Specification
- **Registration**: `{"type": "REGISTER", "username": "...", "password": "..."}`
- **Authentication**: `{"type": "AUTH", "username": "...", "password": "..."}`
- **Broadcast**: `{"type": "BCAST", "content": "..."}`
- **Server Response**: `{"type": "STATUS/MSG", "content/from": "..."}`
*The system utilizes JSON Stream Decoding to handle TCP "sticky packets" during high-load bursts.*

## Compilation and Execution Instructions
### Prerequisites
The system relies on several built-in and third-party libraries. You must ensure the following are installed via pip:

- psutil: Used by the MonitorThread to capture real-time CPU and Memory (RSS) metrics from the server process.
- pandas: Required for processing and cleaning the .csv metric files generated during load tests.
- matplotlib: Necessary for generating the comparative plots (scatter and line graphs) for the Performance Analysis report.
- numpy: Often used as a dependency for data processing within the visualization logic.

### Standard Libraries (No installation required):

- socket: For low-level TCP networking.
- threading / multiprocessing: For handling concurrent clients and background logging.
- json: For the message protocol and stream decoding.
- queue: Powers the Asynchronous Logging Queue.
- argparse: For command-line argument parsing (e.g., --mode, --host).

### Execution Sequence
#### 1. Local Testing (Single Machine)
For initial development and functional verification, you can run all components on 127.0.0.1.

- Start Discovery Server: `py discovery_server.py`

- Start Chat Server:
`py chat_server.py --mode thread  (Options: thread, fork, non-blocking)`

- Launch Clients:
Run `py chat_client.py` in multiple terminals to test messaging functionality.

#### 2. Distributed Testing (Hotspot Environment)
Follow this sequence to perform the performance benchmarking described in Section 4.

##### Step A: Server Setup (Host Laptop)<br>
- Initialize Registry: `py discovery_server.py`

- Start Mode-Specific Server and Monitor:
Use the following command to launch the server and track CPU/Memory metrics:
`py performance_test.py --server-script chat_server.py --server-mode [mode]`
This generates server_metrics_[mode].csv locally.

##### Step B: Client Load Generation (Client Laptop)
- Run Concurrent Load Test:
Ensure the client laptop is connected to the hotspot and run:
`py performance_test.py --host [HOST IP] --server-mode [mode] --num-messages 500`<br>
This simulates 10 concurrent clients and generates client_metrics_[mode].csv on this laptop.

##### Step C: Visualization
- Consolidate Data: Copy the client CSV from the friend's laptop to your project folder.

- Generate Reports: Run the visualization script to create your comparison plots:
`py visualization_script.py`

## Testing Guide
- **Local Testing**: Run all components on `127.0.0.1`.
- **Distributed Testing**: Connect server and client laptops to the same hotspot (e.g., `172.20.10.4`).
- **Load Testing**: Use the `performance_test.py` script to simulate 10 concurrent clients sending a burst of 5,000 total messages.