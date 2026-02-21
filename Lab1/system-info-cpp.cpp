#include <iostream>
#include <cstdlib>

void section(const std::string& title, const std::string& command) {
    std::cout << "\n" << title << ":\n";
    std::cout << "------------------\n";
    system(command.c_str());
}

int main() {
    std::cout << "System Information";

    section("Hostname",         "hostname");
    section("OS Version",       "uname -a");
    section("Distribution",     "cat /etc/os-release | grep PRETTY_NAME");
    section("CPU Info",         "lscpu | grep 'Model name'");
    section("CPU Serial",       "cat /proc/cpuinfo | grep Serial");
    section("CPU Usage",        "top -bn1 | grep 'Cpu(s)'");
    section("Memory",           "free -h");
    section("Disk Usage",       "df -h");
    section("Block Devices",    "lsblk");
    section("Network Interfaces", "ip link | grep link/ether");
    section("IP Addresses",     "ip -4 addr show | grep inet");
    section("Uptime",           "uptime");
    section("Logged-in Users",  "who");

    std::cout << "\n";
    return 0;
}
