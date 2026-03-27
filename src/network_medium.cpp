#include "nicsim/network_medium.h"
#include "nicsim/nic_device.h"
#include <algorithm>

namespace nicsim {

void NetworkMedium::connect(NicDevice* nic) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (std::find(devices_.begin(), devices_.end(), nic) == devices_.end()) {
        devices_.push_back(nic);
        nic->attach(this);
    }
}

void NetworkMedium::disconnect(NicDevice* nic) {
    std::lock_guard<std::mutex> lock(mutex_);
    devices_.erase(std::remove(devices_.begin(), devices_.end(), nic), devices_.end());
    nic->detach();
}

void NetworkMedium::propagate(const EthernetFrame& frame, const NicDevice* sender) {
    std::lock_guard<std::mutex> lock(mutex_);
    for (auto* dev : devices_) {
        if (dev != sender)
            dev->receive(frame);
    }
}

size_t NetworkMedium::device_count() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return devices_.size();
}

}  // namespace nicsim
