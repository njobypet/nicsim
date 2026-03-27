#pragma once

#include "nicsim/ethernet_frame.h"
#include <deque>
#include <mutex>
#include <optional>

namespace nicsim {

class PacketBuffer {
public:
    explicit PacketBuffer(size_t capacity = 256);

    bool enqueue(EthernetFrame frame);
    std::optional<EthernetFrame> dequeue();
    bool empty() const;
    size_t size() const;
    size_t capacity() const { return capacity_; }
    size_t dropped() const { return drop_count_; }

    void clear();

private:
    size_t capacity_;
    size_t drop_count_ = 0;
    mutable std::mutex mutex_;
    std::deque<EthernetFrame> queue_;
};

}  // namespace nicsim
