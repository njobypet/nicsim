#include "nicsim/nic_stats.h"
#include <sstream>

namespace nicsim {

void NicStats::reset() {
    tx_packets = 0;
    rx_packets = 0;
    tx_bytes = 0;
    rx_bytes = 0;
    tx_dropped = 0;
    rx_dropped = 0;
    tx_errors = 0;
    rx_errors = 0;
    collisions = 0;
}

std::string NicStats::to_string() const {
    std::ostringstream oss;
    oss << "NIC Statistics:\n"
        << "  TX packets: " << tx_packets.load() << "  bytes: " << tx_bytes.load() << "\n"
        << "  RX packets: " << rx_packets.load() << "  bytes: " << rx_bytes.load() << "\n"
        << "  TX dropped: " << tx_dropped.load() << "  errors: " << tx_errors.load() << "\n"
        << "  RX dropped: " << rx_dropped.load() << "  errors: " << rx_errors.load() << "\n"
        << "  Collisions: " << collisions.load();
    return oss.str();
}

}  // namespace nicsim
