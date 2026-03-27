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

    NicConfig cfg_sender;
    cfg_sender.name = "sender";
    NicDevice sender(cfg_sender);
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

static void demo_multi_segment() {
    std::cout << "\n=== Multi-Segment Network Demo ===\n\n";

    NicConfig cfg_a;
    cfg_a.name = "seg1_host_a";
    cfg_a.speed_mbps = 1000;

    NicConfig cfg_b;
    cfg_b.name = "seg1_host_b";
    cfg_b.speed_mbps = 1000;

    NicConfig cfg_c;
    cfg_c.name = "seg2_host_c";
    cfg_c.speed_mbps = 1000;

    NicConfig cfg_d;
    cfg_d.name = "seg2_host_d";
    cfg_d.speed_mbps = 1000;

    NicDevice host_a(cfg_a);
    NicDevice host_b(cfg_b);
    NicDevice host_c(cfg_c);
    NicDevice host_d(cfg_d);

    NetworkMedium segment1;
    segment1.connect(&host_a);
    segment1.connect(&host_b);

    NetworkMedium segment2;
    segment2.connect(&host_c);
    segment2.connect(&host_d);

    host_a.link_up();
    host_b.link_up();
    host_c.link_up();
    host_d.link_up();

    std::cout << "Segment 1 (" << segment1.device_count() << " devices):\n";
    std::cout << host_a.info() << "\n\n";
    std::cout << host_b.info() << "\n\n";

    std::cout << "Segment 2 (" << segment2.device_count() << " devices):\n";
    std::cout << host_c.info() << "\n\n";
    std::cout << host_d.info() << "\n\n";

    int seg1_rx = 0, seg2_rx = 0;

    host_b.set_rx_callback([&](const EthernetFrame& f) {
        ++seg1_rx;
        std::cout << "  [Seg1] " << host_b.name() << " received "
                  << f.payload.size() << " bytes from "
                  << f.source.to_string() << "\n";
    });

    host_d.set_rx_callback([&](const EthernetFrame& f) {
        ++seg2_rx;
        std::cout << "  [Seg2] " << host_d.name() << " received "
                  << f.payload.size() << " bytes from "
                  << f.source.to_string() << "\n";
    });

    std::cout << "--- Sending on Segment 1 (A -> B) ---\n";
    {
        EthernetFrame frame;
        frame.destination = host_b.mac();
        frame.ether_type  = EtherType::IPv4;
        frame.payload.assign(128, 0x11);
        host_a.transmit(std::move(frame));
    }

    std::cout << "--- Sending on Segment 2 (C -> D) ---\n";
    {
        EthernetFrame frame;
        frame.destination = host_d.mac();
        frame.ether_type  = EtherType::IPv4;
        frame.payload.assign(256, 0x22);
        host_c.transmit(std::move(frame));
    }

    std::cout << "\n--- Cross-segment isolation test ---\n";
    std::cout << "--- Sending from Seg1 A to Seg2 D's MAC (should NOT arrive) ---\n";
    {
        EthernetFrame frame;
        frame.destination = host_d.mac();
        frame.ether_type  = EtherType::IPv4;
        frame.payload.assign(64, 0x33);
        host_a.transmit(std::move(frame));
    }

    while (host_b.poll_rx()) {}
    while (host_d.poll_rx()) {}

    std::cout << "\nSegment 1 received: " << seg1_rx << " frame(s)\n";
    std::cout << "Segment 2 received: " << seg2_rx << " frame(s)\n";

    std::cout << "\n--- Segment Statistics ---\n";
    std::cout << "[" << host_a.name() << "]\n" << host_a.stats().to_string() << "\n\n";
    std::cout << "[" << host_b.name() << "]\n" << host_b.stats().to_string() << "\n\n";
    std::cout << "[" << host_c.name() << "]\n" << host_c.stats().to_string() << "\n\n";
    std::cout << "[" << host_d.name() << "]\n" << host_d.stats().to_string() << "\n";

    host_a.link_down();
    host_b.link_down();
    host_c.link_down();
    host_d.link_down();
}

int main() {
    demo_two_nics();
    demo_packet_overflow();
    demo_multi_segment();
    return 0;
}
