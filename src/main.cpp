#include "nicsim/nic_device.h"
#include "nicsim/network_medium.h"
#include <iostream>
#include <thread>
#include <chrono>

using namespace nicsim;

static void demo_two_nics() {
    std::cout << "=== NIC Simulator Demo ===\n\n";

    NicConfig cfg_a;
    cfg_a.name = "eth0";
    cfg_a.speed_mbps = 1000;

    NicConfig cfg_b;
    cfg_b.name = "eth1";
    cfg_b.speed_mbps = 1000;

    NicDevice nic_a(cfg_a);
    NicDevice nic_b(cfg_b);

    std::cout << nic_a.info() << "\n\n";
    std::cout << nic_b.info() << "\n\n";

    NetworkMedium segment;
    segment.connect(&nic_a);
    segment.connect(&nic_b);

    nic_a.link_up();
    nic_b.link_up();

    nic_b.set_rx_callback([](const EthernetFrame& f) {
        std::cout << "[" << f.destination.to_string() << " <- "
                  << f.source.to_string() << "] "
                  << f.payload.size() << " byte payload received\n";
    });

    // Send a unicast frame from A to B
    {
        EthernetFrame frame;
        frame.destination = nic_b.mac();
        frame.ether_type  = EtherType::IPv4;
        frame.payload.assign(64, 0xAB);

        std::cout << "--- Sending unicast from " << nic_a.name()
                  << " to " << nic_b.name() << " ---\n";
        nic_a.transmit(std::move(frame));
    }

    // Send a broadcast frame from A
    {
        EthernetFrame frame;
        frame.destination = MacAddress::broadcast();
        frame.ether_type  = EtherType::ARP;
        frame.payload.assign(46, 0x00);

        std::cout << "\n--- Sending broadcast from " << nic_a.name() << " ---\n";
        nic_a.transmit(std::move(frame));
    }

    // Poll any remaining frames on B
    while (auto f = nic_b.poll_rx()) {
        // already handled by callback
    }

    std::cout << "\n--- Final Statistics ---\n";
    std::cout << "[" << nic_a.name() << "]\n" << nic_a.stats().to_string() << "\n\n";
    std::cout << "[" << nic_b.name() << "]\n" << nic_b.stats().to_string() << "\n";

    nic_a.link_down();
    nic_b.link_down();
}

static void demo_packet_overflow() {
    std::cout << "\n=== Packet Buffer Overflow Demo ===\n\n";

    NicConfig cfg;
    cfg.name = "eth_overflow";
    cfg.rx_buf_size = 4;  // tiny buffer

    NicDevice sender((NicConfig{.name = "sender"}));
    NicDevice receiver(cfg);

    NetworkMedium segment;
    segment.connect(&sender);
    segment.connect(&receiver);

    sender.link_up();
    receiver.link_up();

    for (int i = 0; i < 8; ++i) {
        EthernetFrame frame;
        frame.destination = receiver.mac();
        frame.ether_type  = EtherType::IPv4;
        frame.payload.assign(100, static_cast<uint8_t>(i));
        sender.transmit(std::move(frame));
    }

    std::cout << "Sent 8 frames to a NIC with buffer capacity 4.\n";
    std::cout << "[" << receiver.name() << "]\n" << receiver.stats().to_string() << "\n";
}

int main() {
    demo_two_nics();
    demo_packet_overflow();
    return 0;
}
