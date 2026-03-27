#pragma once

#include "nicsim/mac_address.h"
#include <cstdint>
#include <vector>

namespace nicsim {

// Standard EtherType values
enum class EtherType : uint16_t {
    IPv4 = 0x0800,
    ARP  = 0x0806,
    IPv6 = 0x86DD,
    VLAN = 0x8100,
};

struct EthernetFrame {
    MacAddress destination;
    MacAddress source;
    EtherType  ether_type = EtherType::IPv4;
    std::vector<uint8_t> payload;

    static constexpr size_t MIN_PAYLOAD = 46;
    static constexpr size_t MAX_PAYLOAD = 1500;
    static constexpr size_t HEADER_SIZE = 14;   // 6 + 6 + 2
    static constexpr size_t CRC_SIZE    = 4;

    size_t total_size() const { return HEADER_SIZE + payload.size() + CRC_SIZE; }

    std::vector<uint8_t> serialize() const;
    static EthernetFrame deserialize(const std::vector<uint8_t>& data);

    uint32_t compute_crc() const;
};

}  // namespace nicsim
