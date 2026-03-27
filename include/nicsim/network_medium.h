#pragma once

#include "nicsim/ethernet_frame.h"
#include <vector>
#include <mutex>

namespace nicsim {

class NicDevice;

// Simulates a shared Ethernet segment (hub/switch-like medium).
// All connected NICs receive frames sent by any other NIC on the segment.
class NetworkMedium {
public:
    void connect(NicDevice* nic);
    void disconnect(NicDevice* nic);
    void propagate(const EthernetFrame& frame, const NicDevice* sender);
    size_t device_count() const;

private:
    mutable std::mutex mutex_;
    std::vector<NicDevice*> devices_;
};

}  // namespace nicsim
