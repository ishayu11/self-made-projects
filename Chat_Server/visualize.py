import pandas as pd
import matplotlib.pyplot as plt
import glob
import os


def generate_performance_scatter():
    # 1. Setup global plot style
    plt.style.use('seaborn-v0_8-muted')
    client_files = glob.glob("client_metrics_*.csv")

    if not client_files:
        print("Error: No client metrics found. Run the performance test first.")
        return

    plt.figure(figsize=(12, 7))

    # 2. Process Client Latency as a Scatter Plot
    for file in sorted(client_files):
        df = pd.read_csv(file)
        # Identify mode from filename: client_metrics_non-blocking.csv
        mode = os.path.basename(file).split('_')[2].replace(".csv", "").upper()

        # X-axis: Message Count (using the index of the message)
        # Y-axis: Latency in ms
        plt.scatter(df.index, df['latency_ms'], alpha=0.4, s=10, label=f"{mode} Latency")

        # Add a horizontal line for the average to help compare performance
        avg_lat = df['latency_ms'].mean()
        plt.axhline(avg_lat, linestyle='--', linewidth=1.5, label=f"Avg {mode}: {avg_lat:.2f}ms")

    # 3. Labeling for Section 4 Report
    plt.title("Message Delivery Latency Scatter Analysis (Distributed Hotspot)", fontsize=14, fontweight='bold')
    plt.xlabel("Message Count (Sequence 1 to 5000)")  # New X-axis
    plt.ylabel("Latency (ms)")  # New Y-axis
    plt.legend(markerscale=2)  # Increase legend marker size for visibility
    plt.grid(True, alpha=0.3)

    plt.tight_layout()
    plt.savefig("latency_scatter_analysis.png")
    print("Generated: latency_scatter_analysis.png")


if __name__ == "__main__":
    generate_performance_scatter()