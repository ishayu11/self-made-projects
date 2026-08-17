#!/bin/bash

# ==========================================
# ns-3 Assignment Automation Script (Q1 - Q4)
# ==========================================
#
# DOCKER SETUP INSTRUCTIONS (Run this on Windows first!):
# Open Windows Command Prompt or PowerShell and run:
# docker run -it --rm -v D:\Networks\Ass3:/usr/ns3/scratch/combined custom-ns3
#
# Once inside the Docker terminal, execute this script by running:
# chmod +x scratch/combined/run_all.sh
# ./scratch/combined/run_all.sh
# ==========================================

echo "=========================================="
echo " Starting Full Assignment Simulation... "
echo "=========================================="

# ------------------------------------------
# ENVIRONMENT SETUP
# ------------------------------------------
echo "[*] Updating package lists and installing Gnuplot..."
apt-get update
apt-get install -y gnuplot

# Ensure we are in the ns-3 directory
cd /usr/ns3

# Configure ns-3 to make sure it sees all the new folders
echo "[*] Configuring ns-3..."
./ns3 configure

# ------------------------------------------
# QUESTION 1: Single Flow
# ------------------------------------------
echo "[*] Running Q1: Single Flow..."
./ns3 run "scratch/combined/q1/tcp-q1-cwnd"

echo "[*] Generating Q1 Graph..."
gnuplot -e "set term png size 800,600; set output 'scratch/combined/q1/q1_final.png'; set title 'TCP CWND - Single Flow (Q1)'; set xlabel 'Time (seconds)'; set ylabel 'Bytes'; set grid; plot 'scratch/combined/q1/q1_single.cwnd' using 1:3 with lines linewidth 2 lc rgb 'purple' title 'Flow 1'"

# ------------------------------------------
# QUESTION 2: Competing Flows
# ------------------------------------------
echo "[*] Running Q2: Competing Flows..."
./ns3 run "scratch/combined/q2/tcp-q2-cwnd"

echo "[*] Generating Q2 Graph..."
gnuplot -e "set term png size 800,600; set output 'scratch/combined/q2/q2_competing.png'; set title 'TCP CWND - Competing Flows (Q2)'; set xlabel 'Time (seconds)'; set ylabel 'Bytes'; set grid; plot 'scratch/combined/q2/flow1.cwnd' using 1:3 with lines linewidth 2 title 'Flow 1 (n0 -> n2)', 'scratch/combined/q2/flow2.cwnd' using 1:3 with lines linewidth 2 title 'Flow 2 (n1 -> n3)'"

# ------------------------------------------
# QUESTION 3: Bandwidth Thief (3 vs 1)
# ------------------------------------------
echo "[*] Running Q3: Bandwidth Thief..."
./ns3 run "scratch/combined/q3/tcp-q3-cwnd"

echo "[*] Generating Q3 Graph..."
gnuplot -e "set term png size 800,600; set output 'scratch/combined/q3/q3_bandwidth_thief.png'; set title 'TCP CWND - 3 Flows vs 1 Flow (Q3)'; set xlabel 'Time (seconds)'; set ylabel 'Bytes'; set grid; plot 'scratch/combined/q3/flow_n0_0.cwnd' using 1:3 with lines lc rgb 'purple' title 'User 1 - Flow A', 'scratch/combined/q3/flow_n0_1.cwnd' using 1:3 with lines lc rgb 'magenta' title 'User 1 - Flow B', 'scratch/combined/q3/flow_n0_2.cwnd' using 1:3 with lines lc rgb 'blue' title 'User 1 - Flow C', 'scratch/combined/q3/flow_n1.cwnd' using 1:3 with lines linewidth 3 lc rgb 'green' title 'User 2 - Single Flow'"

# ------------------------------------------
# QUESTION 4: Reno vs Cubic
# ------------------------------------------
echo "[*] Running Q4: TCP NewReno..."
./ns3 run "scratch/combined/q4/tcp-q4-cwnd" -- --transport_prot=TcpNewReno
cp scratch/combined/q4/flow_data.cwnd scratch/combined/q4/reno.cwnd

echo "[*] Running Q4: TCP Cubic..."
./ns3 run "scratch/combined/q4/tcp-q4-cwnd" -- --transport_prot=TcpCubic
cp scratch/combined/q4/flow_data.cwnd scratch/combined/q4/cubic.cwnd

echo "[*] Generating Q4 Comparison Graph..."
gnuplot -e "set term png size 800,600; set output 'scratch/combined/q4/reno_vs_cubic.png'; set title 'TCP Reno vs Cubic (High Latency)'; set xlabel 'Time'; set ylabel 'Bytes'; set grid; plot 'scratch/combined/q4/reno.cwnd' using 1:3 with lines linewidth 2 lc rgb 'purple' title 'Reno (Linear)', 'scratch/combined/q4/cubic.cwnd' using 1:3 with lines linewidth 2 lc rgb 'green' title 'Cubic (S-Curve)'"

echo "=========================================="
echo " All simulations complete! Check your folders for the PNG graphs. "
echo "=========================================="