#include "nicsim/ethernet_frame.h"
#include <stdexcept>
#include <numeric>

namespace nicsim {

std::vector<uint8_t> EthernetFrame::serialize() const {
    std::vector<uint8_t> data;
    data.reserve(HEADER_SIZE + payload.size() + CRC_SIZE);

    for (auto b : destination.bytes()) data.push_back(b);
    for (auto b : source.bytes())      data.push_back(b);

    auto et = static_cast<uint16_t>(ether_type);
    data.push_back(static_cast<uint8_t>(et >> 8));
    data.push_back(static_cast<uint8_t>(et & 0xFF));

    data.insert(data.end(), payload.begin(), payload.end());

    uint32_t crc = compute_crc();
    for (int i = 0; i < 4; ++i)
        data.push_back(static_cast<uint8_t>((crc >> (i * 8)) & 0xFF));

    return data;
}

EthernetFrame EthernetFrame::deserialize(const std::vector<uint8_t>& data) {
    if (data.size() < HEADER_SIZE + CRC_SIZE)
        throw std::invalid_argument("Frame too short");

    EthernetFrame frame;

    MacAddress::Bytes dst_bytes, src_bytes;
    std::copy_n(data.begin(), 6, dst_bytes.begin());
    std::copy_n(data.begin() + 6, 6, src_bytes.begin());
    frame.destination = MacAddress(dst_bytes);
    frame.source      = MacAddress(src_bytes);

    frame.ether_type = static_cast<EtherType>(
        (static_cast<uint16_t>(data[12]) << 8) | data[13]);

    size_t payload_len = data.size() - HEADER_SIZE - CRC_SIZE;
    frame.payload.assign(data.begin() + HEADER_SIZE,
                         data.begin() + HEADER_SIZE + payload_len);
    return frame;
}

uint32_t EthernetFrame::compute_crc() const {
    // CRC-32 (IEEE 802.3)
    auto crc_update = [](uint32_t crc, uint8_t byte) {
        crc ^= byte;
        for (int i = 0; i < 8; ++i)
            crc = (crc >> 1) ^ (0xEDB88320 & (-(crc & 1)));
        return crc;
    };

    uint32_t crc = 0xFFFFFFFF;
    for (auto b : destination.bytes()) crc = crc_update(crc, b);
    for (auto b : source.bytes())      crc = crc_update(crc, b);
    auto et = static_cast<uint16_t>(ether_type);
    crc = crc_update(crc, static_cast<uint8_t>(et >> 8));
    crc = crc_update(crc, static_cast<uint8_t>(et & 0xFF));
    for (auto b : payload) crc = crc_update(crc, b);

    return ~crc;
}

}  // namespace nicsim
