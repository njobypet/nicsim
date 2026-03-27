#include "nicsim/mac_address.h"
#include <cstdio>
#include <random>
#include <sstream>
#include <iomanip>
#include <stdexcept>

namespace nicsim {

MacAddress::MacAddress() = default;

MacAddress::MacAddress(const Bytes& bytes) : bytes_(bytes) {}

MacAddress::MacAddress(const std::string& str) {
    unsigned int b[LENGTH];
    if (std::sscanf(str.c_str(), "%02x:%02x:%02x:%02x:%02x:%02x",
                    &b[0], &b[1], &b[2], &b[3], &b[4], &b[5]) != LENGTH) {
        throw std::invalid_argument("Invalid MAC address format: " + str);
    }
    for (size_t i = 0; i < LENGTH; ++i)
        bytes_[i] = static_cast<uint8_t>(b[i]);
}

MacAddress MacAddress::broadcast() {
    return MacAddress(Bytes{0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF});
}

MacAddress MacAddress::random() {
    static std::mt19937 rng{std::random_device{}()};
    std::uniform_int_distribution<int> dist(0, 255);
    Bytes b;
    for (auto& byte : b)
        byte = static_cast<uint8_t>(dist(rng));
    // Set locally-administered and unicast bits
    b[0] = (b[0] & 0xFE) | 0x02;
    return MacAddress(b);
}

std::string MacAddress::to_string() const {
    std::ostringstream oss;
    for (size_t i = 0; i < LENGTH; ++i) {
        if (i > 0) oss << ':';
        oss << std::hex << std::setw(2) << std::setfill('0')
            << static_cast<int>(bytes_[i]);
    }
    return oss.str();
}

bool MacAddress::is_broadcast() const {
    return *this == broadcast();
}

bool MacAddress::is_multicast() const {
    return (bytes_[0] & 0x01) != 0;
}

}  // namespace nicsim
