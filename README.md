# nicsim — Network Interface Card Simulator

A C++17 library and demo application that simulates Ethernet NIC hardware behavior, including MAC addressing, frame serialization, packet buffering, a shared network medium, and — on Linux — a **real virtual network interface** visible to `ifconfig`.

## Features

- **MAC Address** generation (random, broadcast), parsing, and formatting
- **Ethernet Frame** serialization/deserialization with CRC-32 computation
- **Packet Buffer** with configurable capacity and drop tracking (thread-safe)
- **NIC Device** with link up/down, transmit/receive, promiscuous mode, MTU enforcement, and per-device statistics
- **Network Medium** connecting multiple NICs on a shared segment (hub-style broadcast)
- **Multi-Segment Networking** — multiple independent network segments with cross-segment isolation
- **TAP Virtual Interface** (Linux only) — creates a kernel-visible `nicsim0` interface that appears in `ifconfig` / `ip addr`
- **Download Simulation** — injects ~10 MB/s of realistic Ethernet+IPv4+TCP traffic through the TAP interface for 60 seconds (~600 MB), with live progress reporting

## Project Structure

```
nicsim/
├── include/nicsim/    # Public headers
│   ├── mac_address.h
│   ├── ethernet_frame.h
│   ├── packet_buffer.h
│   ├── nic_device.h
│   ├── nic_stats.h
│   ├── network_medium.h
│   └── tap_device.h       # Linux TAP virtual interface
├── src/               # Implementation + main
│   ├── tap_device.cpp     # TAP device (Linux only)
│   └── ...
├── tests/             # Unit tests
└── CMakeLists.txt
```

## Building

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
```

## Running

### Basic demos (any platform)

```bash
./build/nicsim
```

This runs three demos:

1. **Two-NIC Demo** — unicast and broadcast between two NICs on a shared segment
2. **Packet Overflow Demo** — sends 8 frames to a NIC with a 4-frame buffer
3. **Multi-Segment Demo** — two independent network segments with isolation verification

### TAP interface + download simulation (Linux / Ubuntu)

The TAP demo requires root privileges to create the virtual interface:

```bash
sudo ./build/nicsim
```

While it runs, open another terminal to watch the interface:

```bash
# See nicsim0 appear with RX counters climbing in real time
watch -n 1 ifconfig nicsim0

# Or using ip:
watch -n 1 ip -s link show nicsim0
```

The simulation injects ~10 MB/s of traffic for 60 seconds. After it finishes, the interface is torn down automatically.

### Unit tests

```bash
./build/nicsim_tests
```

## Requirements

- CMake 3.16+
- C++17 compiler (GCC 8+, Clang 7+, MSVC 2019+)
- **Linux** for the TAP virtual interface feature (uses `/dev/net/tun`)
- Root/sudo for creating the TAP device

## License

MIT
