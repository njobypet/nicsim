# nicsim - Network Interface Card Simulator

A C++17 library and demo application that simulates Ethernet NIC hardware behavior, including MAC addressing, frame serialization, packet buffering, and a shared network medium.

## Features

- **MAC Address** generation (random, broadcast), parsing, and formatting
- **Ethernet Frame** serialization/deserialization with CRC-32 computation
- **Packet Buffer** with configurable capacity and drop tracking (thread-safe)
- **NIC Device** with link up/down, transmit/receive, promiscuous mode, MTU enforcement, and per-device statistics
- **Network Medium** connecting multiple NICs on a shared segment (hub-style broadcast)

## Project Structure

```
nicsim/
├── include/nicsim/    # Public headers
│   ├── mac_address.h
│   ├── ethernet_frame.h
│   ├── packet_buffer.h
│   ├── nic_device.h
│   ├── nic_stats.h
│   └── network_medium.h
├── src/               # Implementation + main
├── tests/             # Unit tests
└── CMakeLists.txt
```

## Building

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
```

## Running

```bash
# Demo application
./build/nicsim

# Unit tests
./build/nicsim_tests
```

## Requirements

- CMake 3.16+
- C++17 compiler (MSVC 2019+, GCC 8+, Clang 7+)

## License

MIT
