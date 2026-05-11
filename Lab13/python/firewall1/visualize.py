import json
import sys
import glob
import os
import re
from collections import Counter

import matplotlib.pyplot as plt

PORT_PROTOCOLS = {
    80: "HTTP",
    443: "HTTPS",
    21: "FTP",
    22: "SSH",
    23: "Telnet",
}


def find_latest_log(directory):
    candidates = sorted(
        glob.glob(os.path.join(directory, "firewall_log_*.json")),
        key=os.path.getmtime,
        reverse=True,
    )
    for path in candidates:
        try:
            with open(path, "r", encoding="utf-8") as f:
                json.load(f)
            return path
        except (json.JSONDecodeError, OSError):
            print(f"Skipping invalid log: {os.path.basename(path)}")
    return None


def load_log(path):
    with open(path, "r", encoding="utf-8") as f:
        return json.load(f)


def plot_heavy_ports(ax, heavy_ports, blocked_in, blocked_out):
    if not heavy_ports:
        ax.set_title("Heavy ports (no data)")
        return

    ports = [int(p) for p, _ in heavy_ports]
    counts = [c for _, c in heavy_ports]
    labels = [f"{p}\n{PORT_PROTOCOLS.get(p, 'TCP/UDP')}" for p in ports]
    colors = [
        "red" if (p in blocked_in or p in blocked_out) else "steelblue"
        for p in ports
    ]

    bars = ax.bar(labels, counts, color=colors)
    ax.set_title("Heavy ports (red = blocked by firewall rule)")
    ax.set_ylabel("Hit count")
    ax.set_xlabel("Port / Protocol")

    for bar, count in zip(bars, counts):
        ax.text(
            bar.get_x() + bar.get_width() / 2,
            bar.get_height(),
            str(count),
            ha="center",
            va="bottom",
        )


def plot_protocol_distribution(ax, open_ports):
    proto_counts = Counter()
    for entry in open_ports:
        port = entry.get("port")
        proto = PORT_PROTOCOLS.get(port, entry.get("protocol", "TCP/UDP"))
        proto_counts[proto] += 1

    if not proto_counts:
        ax.set_title("Protocol distribution (no data)")
        return

    labels = list(proto_counts.keys())
    sizes = list(proto_counts.values())
    ax.pie(sizes, labels=labels, autopct="%1.1f%%", startangle=90)
    ax.set_title("Protocol distribution of open ports")


def plot_status_distribution(ax, open_ports):
    status_counts = Counter(entry.get("status", "UNKNOWN") for entry in open_ports)
    if not status_counts:
        ax.set_title("Connection status (no data)")
        return

    statuses = list(status_counts.keys())
    counts = [status_counts[s] for s in statuses]
    ax.bar(statuses, counts, color="seagreen")
    ax.set_title("Connection status distribution")
    ax.set_ylabel("Count")
    ax.tick_params(axis="x", rotation=30)


def output_path_for(log_path):
    base = os.path.basename(log_path)
    m = re.match(r"firewall_log_(.+)\.json$", base)
    stamp = m.group(1) if m else "out"
    return os.path.join(os.path.dirname(log_path), f"firewall_chart_{stamp}.png")


def main():
    if len(sys.argv) > 1:
        log_path = sys.argv[1]
    else:
        log_path = find_latest_log(os.path.dirname(os.path.abspath(__file__)))
        if log_path is None:
            print("No firewall_log_*.json file found in this directory.")
            sys.exit(1)

    print(f"Loading log: {log_path}")
    data = load_log(log_path)

    open_ports = data.get("open_ports", [])
    heavy_ports = data.get("heavy_ports", [])
    rules = data.get("rules", {})
    blocked_in = set(rules.get("blocked_in", []))
    blocked_out = set(rules.get("blocked_out", []))

    fig, axes = plt.subplots(2, 2, figsize=(13, 9))
    fig.suptitle(f"Firewall report — {os.path.basename(log_path)}", fontsize=14)

    plot_heavy_ports(axes[0, 0], heavy_ports, blocked_in, blocked_out)
    plot_protocol_distribution(axes[0, 1], open_ports)
    plot_status_distribution(axes[1, 0], open_ports)

    axes[1, 1].axis("off")
    rules_text = (
        f"Firewall rules\n"
        f"---------------\n"
        f"Blocked IN  : {sorted(blocked_in) or '—'}\n"
        f"Blocked OUT : {sorted(blocked_out) or '—'}\n\n"
        f"Open ports total : {len(open_ports)}\n"
        f"Heavy ports shown: {len(heavy_ports)}"
    )
    axes[1, 1].text(0.05, 0.95, rules_text, va="top", ha="left",
                    family="monospace", fontsize=11)

    plt.tight_layout(rect=[0, 0, 1, 0.96])

    out_path = output_path_for(log_path)
    plt.savefig(out_path, dpi=120)
    print(f"Saved chart: {out_path}")

    plt.show()


if __name__ == "__main__":
    main()
