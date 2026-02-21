#!/bin/bash

DIVIDER="------------------"

echo "System Information"
echo "$DIVIDER"

echo "Hostname:"
hostname
echo "$DIVIDER"

echo "OS Version:"
uname -a
echo "$DIVIDER"

echo "Distribution:"
cat /etc/os-release | grep PRETTY_NAME
echo "$DIVIDER"

echo "CPU Info:"
lscpu | grep "Model name"
echo "$DIVIDER"

echo "CPU Serial:"
cat /proc/cpuinfo | grep Serial
echo "$DIVIDER"

echo "CPU Usage:"
top -bn1 | grep "Cpu(s)"
echo "$DIVIDER"

echo "Memory:"
free -h
echo "$DIVIDER"

echo "Disk Usage:"
df -h
echo "$DIVIDER"

echo "Block Devices:"
lsblk
echo "$DIVIDER"

echo "Network Interfaces:"
ip link | grep link/ether
echo "$DIVIDER"

echo "IP Addresses:"
ip -4 addr show | grep inet
echo "$DIVIDER"

echo "Uptime:"
uptime
echo "$DIVIDER"

echo "Logged-in Users:"
who
echo "$DIVIDER"

