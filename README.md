# nicsim — Network Interface Card Simulator

A C++17 library that simulates Ethernet NIC hardware behavior and — on Linux — creates a **real virtual network interface** (`nicsim0`) visible to `ifconfig`. A companion `test_nicsim` CLI drives simulations against the running server.

## Features

- **MAC Address** generation (random, broadcast), parsing, and formatting
- **Ethernet Frame** serialization/deserialization with CRC-32 computation
- **Packet Buffer** with configurable capacity and drop tracking (thread-safe)
- **NIC Device** with link up/down, transmit/receive, promiscuous mode, MTU enforcement, and per-device statistics
- **Network Medium** connecting multiple NICs on a shared segment (hub-style broadcast)
- **Multi-Segment Networking** — independent network segments with cross-segment isolation
- **TAP Virtual Interface** (Linux) — kernel-visible `nicsim0` that appears in `ifconfig` / `ip addr`
- **test_nicsim CLI** — sends commands to the running server:
  - `--ping` — 10 ICMP echo replies with per-packet timing
  - `--icmp` — inject various ICMP packet types (echo, unreachable, time-exceeded)
  - `--download --1gb` — simulate a large file download at ~10 MB/s with live progress
  - `--exit` — gracefully shut down the server

## Project Structure

```
nicsim/
├── include/nicsim/        # Public headers
│   ├── mac_address.h
│   ├── ethernet_frame.h
│   ├── packet_buffer.h
│   ├── nic_device.h
│   ├── nic_stats.h
│   ├── network_medium.h
│   └── tap_device.h       # Linux TAP virtual interface
├── src/
│   ├── main.cpp           # TAP server (Linux) / demos (other)
│   ├── test_nicsim.cpp    # CLI client for the server
│   ├── tap_device.cpp     # TAP device implementation
│   └── ...
├── tests/
│   └── test_main.cpp      # Unit tests
└── CMakeLists.txt
```

## Building

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
```

This produces three binaries:
- `nicsim` — main server / demo application
- `test_nicsim` — CLI client (Linux/Unix only)
- `nicsim_tests` — unit tests

## Running on Linux / Ubuntu

### 1. Start the server

```bash
sudo ./build/nicsim
```

The server creates the `nicsim0` TAP interface, assigns IP `10.0.100.1/24`, and waits for commands over a Unix socket. The interface stays alive as long as the server is running.

Verify the interface in another terminal:

```bash
ifconfig nicsim0
# or
ip addr show nicsim0
```

### 2. Run simulations with test_nicsim

**Simulate ping:**

```bash
./build/test_nicsim --ping
```

Output:

```
PING 203.0.113.100 via nicsim0: 56 data bytes
98 bytes from 203.0.113.100: icmp_seq=1 ttl=64 time=0.1 ms
98 bytes from 203.0.113.100: icmp_seq=2 ttl=64 time=0.1 ms
...
--- 203.0.113.100 ping statistics ---
10 packets transmitted, 10 received, 0% packet loss
```

**Simulate ICMP traffic:**

```bash
./build/test_nicsim --icmp
```

Injects Echo Requests, Echo Replies, Destination Unreachable, and Time Exceeded packets.

**Simulate a large file download:**

```bash
./build/test_nicsim --download --1gb
./build/test_nicsim --download --500mb
./build/test_nicsim --download --100mb
```

Injects Ethernet+IPv4+TCP frames at ~10 MB/s. Watch the RX counters climb:

```bash
watch -n 1 ifconfig nicsim0
```

**Stop the server:**

```bash
./build/test_nicsim --exit
```

### 3. Running on other platforms

On non-Linux systems, `nicsim` runs the built-in userspace demos (two-NIC, packet overflow, multi-segment). The TAP and `test_nicsim` features require Linux.

### Unit tests

```bash
./build/nicsim_tests
```

## Requirements

- CMake 3.16+
- C++17 compiler (GCC 8+, Clang 7+, MSVC 2019+)
- **Linux** for the TAP virtual interface and test_nicsim (uses `/dev/net/tun`)
- Root/sudo for creating the TAP device

## License

MIT
