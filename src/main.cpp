#include "nicsim/nic_device.h"
#include "nicsim/network_medium.h"
#include <iostream>
#include <thread>
#include <chrono>
#include <iomanip>
#include <vector>

#ifdef __linux__
#include "nicsim/tap_device.h"
#include <cstring>
#include <arpa/inet.h>
#endif

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

#ifdef __linux__

static uint16_t ip_checksum(const void* buf, size_t len) {
    auto p = static_cast<const uint16_t*>(buf);
    uint32_t sum = 0;
    while (len > 1) {
        sum += *p++;
        len -= 2;
    }
    if (len == 1)
        sum += *reinterpret_cast<const uint8_t*>(p);
    while (sum >> 16)
        sum = (sum & 0xFFFF) + (sum >> 16);
    return static_cast<uint16_t>(~sum);
}

static std::vector<uint8_t> build_tcp_data_frame(
        uint32_t tcp_seq, uint16_t ip_id, size_t payload_size) {
    const size_t ETH_HDR = 14, IP_HDR = 20, TCP_HDR = 20;
    std::vector<uint8_t> f(ETH_HDR + IP_HDR + TCP_HDR + payload_size, 0);
    uint8_t* p = f.data();

    // -- Ethernet header --
    const uint8_t dst_mac[] = {0x02, 0x00, 0x0a, 0x00, 0x64, 0x01};
    const uint8_t src_mac[] = {0x02, 0x00, 0xcb, 0x00, 0x71, 0x64};
    std::memcpy(p, dst_mac, 6);
    std::memcpy(p + 6, src_mac, 6);
    p[12] = 0x08; p[13] = 0x00;

    // -- IPv4 header --
    uint8_t* ip = p + ETH_HDR;
    ip[0] = 0x45;
    uint16_t tot = htons(static_cast<uint16_t>(IP_HDR + TCP_HDR + payload_size));
    std::memcpy(ip + 2, &tot, 2);
    uint16_t id_n = htons(ip_id);
    std::memcpy(ip + 4, &id_n, 2);
    ip[6] = 0x40;
    ip[8] = 64;
    ip[9] = 6;
    uint32_t sip = htonl(0xCB007164);   // 203.0.113.100
    uint32_t dip = htonl(0x0A006401);   // 10.0.100.1
    std::memcpy(ip + 12, &sip, 4);
    std::memcpy(ip + 16, &dip, 4);
    uint16_t ck = ip_checksum(ip, IP_HDR);
    std::memcpy(ip + 10, &ck, 2);

    // -- TCP header --
    uint8_t* tcp = ip + IP_HDR;
    uint16_t sp = htons(443), dp = htons(49152);
    std::memcpy(tcp, &sp, 2);
    std::memcpy(tcp + 2, &dp, 2);
    uint32_t seq_n = htonl(tcp_seq);
    std::memcpy(tcp + 4, &seq_n, 4);
    uint32_t ack_n = htonl(1);
    std::memcpy(tcp + 8, &ack_n, 4);
    tcp[12] = 0x50;
    tcp[13] = 0x18;     // PSH | ACK
    uint16_t win = htons(65535);
    std::memcpy(tcp + 14, &win, 2);

    // -- Payload --
    uint8_t* payload = tcp + TCP_HDR;
    for (size_t i = 0; i < payload_size; ++i)
        payload[i] = static_cast<uint8_t>(i);

    return f;
}

static void demo_tap_download() {
    using namespace nicsim;

    std::cout << "\n=== Virtual Interface — Large File Download ===\n\n";
    std::cout << "Creating TAP device 'nicsim0' ...\n";

    TapDevice tap("nicsim0");
    if (!tap.create()) {
        std::cerr << "ERROR: could not create TAP device.  Run with:  sudo ./nicsim\n";
        return;
    }

    if (!tap.configure("10.0.100.1")) {
        std::cerr << "ERROR: could not assign IP address\n";
        return;
    }

    if (!tap.bring_up()) {
        std::cerr << "ERROR: could not bring interface up\n";
        return;
    }

    std::cout << "Interface '" << tap.dev_name() << "' is UP — 10.0.100.1/24\n";
    std::cout << ">>> Open another terminal and run:\n";
    std::cout << ">>>   ifconfig " << tap.dev_name() << "\n";
    std::cout << ">>>   (or: ip -s link show " << tap.dev_name() << ")\n\n";

    const size_t   PAYLOAD     = 1400;
    const size_t   FRAME_SIZE  = 14 + 20 + 20 + PAYLOAD;  // 1454
    const double   TARGET_MBs  = 10.0;
    const int      DURATION_S  = 60;
    const size_t   FPS         = static_cast<size_t>(TARGET_MBs * 1e6 / FRAME_SIZE);
    const size_t   BATCH       = FPS / 20;   // ~50 ms worth

    std::cout << "Simulating download:  203.0.113.100:443  -->  10.0.100.1:49152\n";
    std::cout << "Target rate : ~" << static_cast<int>(TARGET_MBs) << " MB/s   "
              << "Duration : " << DURATION_S << " s\n\n";

    uint32_t tcp_seq = 1;
    uint16_t ip_id   = 1;
    uint64_t total_bytes  = 0;
    uint64_t total_frames = 0;

    auto t_start  = std::chrono::steady_clock::now();
    auto t_report = t_start;
    uint64_t report_bytes = 0;

    // Pre-build a template frame and mutate seq/id each iteration
    auto frame = build_tcp_data_frame(tcp_seq, ip_id, PAYLOAD);

    while (true) {
        auto now = std::chrono::steady_clock::now();
        double elapsed = std::chrono::duration<double>(now - t_start).count();
        if (elapsed >= DURATION_S) break;

        for (size_t i = 0; i < BATCH; ++i) {
            // Patch TCP sequence number  (offset 38 = 14+20+4)
            uint32_t sn = htonl(tcp_seq);
            std::memcpy(frame.data() + 38, &sn, 4);

            // Patch IP identification    (offset 18 = 14+4)
            uint16_t id_n = htons(ip_id);
            std::memcpy(frame.data() + 18, &id_n, 2);

            // Recompute IP checksum
            std::memset(frame.data() + 24, 0, 2);
            uint16_t ck = ip_checksum(frame.data() + 14, 20);
            std::memcpy(frame.data() + 24, &ck, 2);

            ssize_t w = tap.inject(frame.data(), frame.size());
            if (w > 0) {
                total_bytes  += static_cast<uint64_t>(w);
                total_frames++;
                tcp_seq += static_cast<uint32_t>(PAYLOAD);
                ip_id++;
            }
        }

        // Progress report every second
        double since = std::chrono::duration<double>(now - t_report).count();
        if (since >= 1.0) {
            double speed = static_cast<double>(total_bytes - report_bytes) / since / 1e6;
            std::cout << "  [" << std::setw(3) << static_cast<int>(elapsed) << "s]  "
                      << std::setw(8) << total_frames << " frames  |  "
                      << std::setw(6) << (total_bytes / 1000000) << " MB  |  "
                      << std::fixed << std::setprecision(1) << speed << " MB/s\n";
            t_report     = now;
            report_bytes = total_bytes;
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }

    std::cout << "\n--- Download Complete ---\n"
              << "  Duration     : " << DURATION_S << " s\n"
              << "  Frames       : " << total_frames << "\n"
              << "  Total data   : " << (total_bytes / 1000000) << " MB\n"
              << "  Avg speed    : " << std::fixed << std::setprecision(1)
              << (static_cast<double>(total_bytes) / 1e6 / DURATION_S) << " MB/s\n\n";

    std::cout << ">>> Check final stats:  ifconfig " << tap.dev_name() << "\n";
    std::cout << "Tearing down interface ...\n";
}

#endif  // __linux__

int main() {
    demo_two_nics();
    demo_packet_overflow();
    demo_multi_segment();

#ifdef __linux__
    demo_tap_download();
#endif

    return 0;
}
