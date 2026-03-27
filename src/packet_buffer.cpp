#include "nicsim/packet_buffer.h"

namespace nicsim {

PacketBuffer::PacketBuffer(size_t capacity) : capacity_(capacity) {}

bool PacketBuffer::enqueue(EthernetFrame frame) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (queue_.size() >= capacity_) {
        ++drop_count_;
        return false;
    }
    queue_.push_back(std::move(frame));
    return true;
}

std::optional<EthernetFrame> PacketBuffer::dequeue() {
    std::lock_guard<std::mutex> lock(mutex_);
    if (queue_.empty())
        return std::nullopt;
    auto frame = std::move(queue_.front());
    queue_.pop_front();
    return frame;
}

bool PacketBuffer::empty() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return queue_.empty();
}

size_t PacketBuffer::size() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return queue_.size();
}

void PacketBuffer::clear() {
    std::lock_guard<std::mutex> lock(mutex_);
    queue_.clear();
}

}  // namespace nicsim
