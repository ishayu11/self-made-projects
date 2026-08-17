import matplotlib.pyplot as plt
import os

base_path = "" 
variants = [
    {"label": "TCP Cubic",   "file": "ns3-projects/tcp-example-TcpCubic.cwnd",   "color": "blue"},
    {"label": "TCP BIC",     "file": "ns3-projects/tcp-example-TcpBic.cwnd",     "color": "red"},
    {"label": "TCP NewReno", "file": "ns3-projects/tcp-example-TcpNewReno.cwnd", "color": "green"}
]

plt.figure(figsize=(12, 7))
plotted_count = 0

for variant in variants:
    times = []
    cwnds = []
    filepath = os.path.join(base_path, variant["file"])
    
    try:
        with open(filepath, 'r') as file:
            for line in file:
                parts = line.split()
                if len(parts) >= 3:
                    times.append(float(parts[0]))
                    cwnds.append(float(parts[2])) # This is the New Window Size
        plt.plot(times, cwnds, label=variant["label"], color=variant["color"], linewidth=1.5, alpha=0.8)
        plotted_count += 1
        print(f"Successfully loaded: {filepath}")
        
    except FileNotFoundError:
        print(f"Warning: Could not find '{filepath}'. Skipping...")

if plotted_count > 0:
    plt.title("TCP Congestion Window Comparison")
    plt.xlabel("Simulation Time (Seconds)")
    plt.ylabel("Congestion Window Size (Bytes)")
    plt.grid(True, linestyle='--', alpha=0.7)
    plt.legend()
    output_image = "tcp_comparison_graph.png"
    plt.savefig(output_image, dpi=300)
    print(f"\nSuccess! Combined graph saved as '{output_image}'")
    plt.show()
else:
    print("\nError: No .cwnd files were found to plot. Please check your file paths!")