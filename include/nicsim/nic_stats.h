#pragma once

#include <atomic>
#include <cstdint>
#include <string>

namespace nicsim {

struct NicStats {
    std::atomic<uint64_t> tx_packets{0};
    std::atomic<uint64_t> rx_packets{0};
    std::atomic<uint64_t> tx_bytes{0};
    std::atomic<uint64_t> rx_bytes{0};
    std::atomic<uint64_t> tx_dropped{0};
    std::atomic<uint64_t> rx_dropped{0};
    std::atomic<uint64_t> tx_errors{0};
    std::atomic<uint64_t> rx_errors{0};
    std::atomic<uint64_t> collisions{0};

    void reset();
    std::string to_string() const;
};

}  // namespace nicsim
