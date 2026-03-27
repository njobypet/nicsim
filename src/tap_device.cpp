#ifdef __linux__

#include "nicsim/tap_device.h"

#include <cstring>
#include <iostream>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <net/if.h>
#include <linux/if_tun.h>

namespace nicsim {

TapDevice::TapDevice(const std::string& dev_name) : dev_name_(dev_name) {}

TapDevice::~TapDevice() {
    destroy();
}

bool TapDevice::create() {
    fd_ = ::open("/dev/net/tun", O_RDWR);
    if (fd_ < 0) {
        std::cerr << "Failed to open /dev/net/tun — run with sudo\n";
        return false;
    }

    struct ifreq ifr{};
    ifr.ifr_flags = IFF_TAP | IFF_NO_PI;
    std::strncpy(ifr.ifr_name, dev_name_.c_str(), IFNAMSIZ - 1);

    if (::ioctl(fd_, TUNSETIFF, &ifr) < 0) {
        std::cerr << "ioctl TUNSETIFF failed\n";
        ::close(fd_);
        fd_ = -1;
        return false;
    }

    dev_name_ = ifr.ifr_name;
    return true;
}

void TapDevice::destroy() {
    if (fd_ >= 0) {
        bring_down();
        ::close(fd_);
        fd_ = -1;
    }
}

bool TapDevice::configure(const std::string& ip, const std::string& netmask) {
    int sock = ::socket(AF_INET, SOCK_DGRAM, 0);
    if (sock < 0) return false;

    struct ifreq ifr{};
    std::strncpy(ifr.ifr_name, dev_name_.c_str(), IFNAMSIZ - 1);

    auto* addr = reinterpret_cast<struct sockaddr_in*>(&ifr.ifr_addr);
    addr->sin_family = AF_INET;

    ::inet_pton(AF_INET, ip.c_str(), &addr->sin_addr);
    if (::ioctl(sock, SIOCSIFADDR, &ifr) < 0) {
        ::close(sock);
        return false;
    }

    ::inet_pton(AF_INET, netmask.c_str(), &addr->sin_addr);
    if (::ioctl(sock, SIOCSIFNETMASK, &ifr) < 0) {
        ::close(sock);
        return false;
    }

    ::close(sock);
    return true;
}

bool TapDevice::bring_up() {
    int sock = ::socket(AF_INET, SOCK_DGRAM, 0);
    if (sock < 0) return false;

    struct ifreq ifr{};
    std::strncpy(ifr.ifr_name, dev_name_.c_str(), IFNAMSIZ - 1);

    if (::ioctl(sock, SIOCGIFFLAGS, &ifr) < 0) {
        ::close(sock);
        return false;
    }

    ifr.ifr_flags |= IFF_UP | IFF_RUNNING;
    if (::ioctl(sock, SIOCSIFFLAGS, &ifr) < 0) {
        ::close(sock);
        return false;
    }

    ::close(sock);
    return true;
}

bool TapDevice::bring_down() {
    int sock = ::socket(AF_INET, SOCK_DGRAM, 0);
    if (sock < 0) return false;

    struct ifreq ifr{};
    std::strncpy(ifr.ifr_name, dev_name_.c_str(), IFNAMSIZ - 1);

    if (::ioctl(sock, SIOCGIFFLAGS, &ifr) < 0) {
        ::close(sock);
        return false;
    }

    ifr.ifr_flags &= ~(IFF_UP | IFF_RUNNING);
    if (::ioctl(sock, SIOCSIFFLAGS, &ifr) < 0) {
        ::close(sock);
        return false;
    }

    ::close(sock);
    return true;
}

ssize_t TapDevice::inject(const void* data, size_t len) {
    if (fd_ < 0) return -1;
    return ::write(fd_, data, len);
}

}  // namespace nicsim

#endif  // __linux__
