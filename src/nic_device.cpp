#include "nicsim/nic_device.h"
#include "nicsim/network_medium.h"
#include <sstream>

namespace nicsim {

NicDevice::NicDevice(const NicConfig& config)
    : config_(config),
      rx_buffer_(config.rx_buf_size),
      tx_buffer_(config.tx_buf_size) {}

NicDevice::~NicDevice() {
    if (medium_)
        medium_->disconnect(this);
}

void NicDevice::link_up() {
    state_ = NicState::UP;
}

void NicDevice::link_down() {
    state_ = NicState::DOWN;
    rx_buffer_.clear();
    tx_buffer_.clear();
}

bool NicDevice::transmit(EthernetFrame frame) {
    if (state_ != NicState::UP) {
        stats_.tx_errors++;
        return false;
    }

    if (frame.payload.size() > config_.mtu) {
        stats_.tx_errors++;
        return false;
    }

    frame.source = config_.mac;

    stats_.tx_packets++;
    stats_.tx_bytes += frame.total_size();

    if (medium_)
        medium_->propagate(frame, this);

    return true;
}

void NicDevice::receive(const EthernetFrame& frame) {
    if (state_ != NicState::UP)
        return;

    if (!should_accept(frame)) {
        return;
    }

    stats_.rx_packets++;
    stats_.rx_bytes += frame.total_size();

    if (!rx_buffer_.enqueue(frame)) {
        stats_.rx_dropped++;
        return;
    }

    if (rx_callback_)
        rx_callback_(frame);
}

std::optional<EthernetFrame> NicDevice::poll_rx() {
    return rx_buffer_.dequeue();
}

bool NicDevice::should_accept(const EthernetFrame& frame) const {
    if (config_.promiscuous)
        return true;
    if (frame.destination.is_broadcast())
        return true;
    return frame.destination == config_.mac;
}

std::string NicDevice::info() const {
    std::ostringstream oss;
    oss << "NIC [" << config_.name << "]\n"
        << "  MAC:    " << config_.mac.to_string() << "\n"
        << "  State:  " << (state_ == NicState::UP ? "UP" : "DOWN") << "\n"
        << "  Speed:  " << config_.speed_mbps << " Mbps "
        << (config_.duplex == DuplexMode::FULL ? "full-duplex" : "half-duplex") << "\n"
        << "  MTU:    " << config_.mtu << "\n"
        << "  Promisc:" << (config_.promiscuous ? " on" : " off") << "\n"
        << "  RX buf: " << rx_buffer_.size() << "/" << rx_buffer_.capacity() << "\n"
        << "  TX buf: " << tx_buffer_.size() << "/" << tx_buffer_.capacity();
    return oss.str();
}

}  // namespace nicsim
