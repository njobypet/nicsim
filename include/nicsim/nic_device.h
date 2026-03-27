#pragma once

#include "nicsim/mac_address.h"
#include "nicsim/ethernet_frame.h"
#include "nicsim/packet_buffer.h"
#include "nicsim/nic_stats.h"
#include <functional>
#include <string>

namespace nicsim {

class NetworkMedium;

enum class NicState { DOWN, UP };
enum class DuplexMode { HALF, FULL };

struct NicConfig {
    std::string   name        = "eth0";
    MacAddress    mac         = MacAddress::random();
    size_t        mtu         = 1500;
    size_t        rx_buf_size = 256;
    size_t        tx_buf_size = 256;
    uint32_t      speed_mbps  = 1000;
    DuplexMode    duplex      = DuplexMode::FULL;
    bool          promiscuous = false;
};

using RxCallback = std::function<void(const EthernetFrame&)>;

class NicDevice {
public:
    explicit NicDevice(const NicConfig& config = {});
    ~NicDevice();

    NicDevice(const NicDevice&) = delete;
    NicDevice& operator=(const NicDevice&) = delete;

    void link_up();
    void link_down();
    NicState state() const { return state_; }

    bool transmit(EthernetFrame frame);
    void receive(const EthernetFrame& frame);
    std::optional<EthernetFrame> poll_rx();

    void set_rx_callback(RxCallback cb) { rx_callback_ = std::move(cb); }
    void attach(NetworkMedium* medium) { medium_ = medium; }
    void detach() { medium_ = nullptr; }

    const NicConfig&  config() const { return config_; }
    const NicStats&   stats()  const { return stats_; }
    const MacAddress& mac()    const { return config_.mac; }
    const std::string& name()  const { return config_.name; }

    void set_promiscuous(bool on) { config_.promiscuous = on; }
    std::string info() const;

private:
    bool should_accept(const EthernetFrame& frame) const;

    NicConfig      config_;
    NicState       state_ = NicState::DOWN;
    PacketBuffer   rx_buffer_;
    PacketBuffer   tx_buffer_;
    NicStats       stats_;
    NetworkMedium* medium_ = nullptr;
    RxCallback     rx_callback_;
};

}  // namespace nicsim
