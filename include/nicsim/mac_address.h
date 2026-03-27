#pragma once

#include <array>
#include <cstdint>
#include <string>
#include <functional>

namespace nicsim {

class MacAddress {
public:
    static constexpr size_t LENGTH = 6;
    using Bytes = std::array<uint8_t, LENGTH>;

    MacAddress();
    explicit MacAddress(const Bytes& bytes);
    explicit MacAddress(const std::string& str);

    static MacAddress broadcast();
    static MacAddress random();

    const Bytes& bytes() const { return bytes_; }
    std::string to_string() const;
    bool is_broadcast() const;
    bool is_multicast() const;

    bool operator==(const MacAddress& other) const { return bytes_ == other.bytes_; }
    bool operator!=(const MacAddress& other) const { return !(*this == other); }

private:
    Bytes bytes_{};
};

}  // namespace nicsim

namespace std {
template <>
struct hash<nicsim::MacAddress> {
    size_t operator()(const nicsim::MacAddress& addr) const {
        size_t h = 0;
        for (auto b : addr.bytes())
            h = h * 31 + b;
        return h;
    }
};
}  // namespace std
