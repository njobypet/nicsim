#pragma once

#ifdef __linux__

#include <string>
#include <cstddef>
#include <sys/types.h>

namespace nicsim {

// Wraps a Linux TAP device (/dev/net/tun) to create a kernel-visible
// virtual network interface that appears in ifconfig / ip addr.
class TapDevice {
public:
    explicit TapDevice(const std::string& dev_name = "nicsim0");
    ~TapDevice();

    TapDevice(const TapDevice&) = delete;
    TapDevice& operator=(const TapDevice&) = delete;

    bool create();
    void destroy();
    bool configure(const std::string& ip,
                   const std::string& netmask = "255.255.255.0");
    bool bring_up();
    bool bring_down();

    // Write a raw Ethernet frame into the interface (appears as RX traffic).
    ssize_t inject(const void* data, size_t len);

    const std::string& dev_name() const { return dev_name_; }
    int fd() const { return fd_; }
    bool is_open() const { return fd_ >= 0; }

private:
    std::string dev_name_;
    int fd_ = -1;
};

}  // namespace nicsim

#endif  // __linux__
