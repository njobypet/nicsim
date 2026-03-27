#include "nicsim/nic_device.h"
#include "nicsim/network_medium.h"
#include <iostream>
#include <thread>
#include <chrono>
#include <iomanip>
#include <vector>
#include <string>
#include <sstream>
#include <csignal>

#ifdef __linux__
#include "nicsim/tap_device.h"
#include <cstring>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <poll.h>
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

// ---- Linux TAP server ----

#ifdef __linux__

static constexpr const char* SOCKET_PATH = "/tmp/nicsim.sock";
static volatile sig_atomic_t g_exit_flag = 0;
static void on_signal(int) { g_exit_flag = 1; }

static void respond(int fd, const std::string& msg) {
    ::send(fd, msg.c_str(), msg.size(), MSG_NOSIGNAL);
}

static uint16_t ip_checksum(const void* buf, size_t len) {
    auto p = static_cast<const uint16_t*>(buf);
    uint32_t sum = 0;
    while (len > 1) { sum += *p++; len -= 2; }
    if (len == 1) sum += *reinterpret_cast<const uint8_t*>(p);
    while (sum >> 16) sum = (sum & 0xFFFF) + (sum >> 16);
    return static_cast<uint16_t>(~sum);
}

static void fill_eth_ip(uint8_t* frame, uint8_t protocol,
                         uint16_t ip_id, uint16_t ip_payload_len) {
    const uint8_t dst_mac[] = {0x02, 0x00, 0x0a, 0x00, 0x64, 0x01};
    const uint8_t src_mac[] = {0x02, 0x00, 0xcb, 0x00, 0x71, 0x64};
    std::memcpy(frame, dst_mac, 6);
    std::memcpy(frame + 6, src_mac, 6);
    frame[12] = 0x08; frame[13] = 0x00;

    uint8_t* ip = frame + 14;
    ip[0] = 0x45;
    uint16_t tot = htons(static_cast<uint16_t>(20 + ip_payload_len));
    std::memcpy(ip + 2, &tot, 2);
    uint16_t idn = htons(ip_id);
    std::memcpy(ip + 4, &idn, 2);
    ip[6] = 0x40;
    ip[8] = 64;
    ip[9] = protocol;
    uint32_t sip = htonl(0xCB007164);   // 203.0.113.100
    uint32_t dip = htonl(0x0A006401);   // 10.0.100.1
    std::memcpy(ip + 12, &sip, 4);
    std::memcpy(ip + 16, &dip, 4);
    uint16_t ck = ip_checksum(ip, 20);
    std::memcpy(ip + 10, &ck, 2);
}

static std::vector<uint8_t> build_tcp_frame(uint32_t seq, uint16_t ip_id,
                                             size_t payload_sz) {
    const size_t TCP_HDR = 20;
    std::vector<uint8_t> f(34 + TCP_HDR + payload_sz, 0);
    fill_eth_ip(f.data(), 6, ip_id, static_cast<uint16_t>(TCP_HDR + payload_sz));

    uint8_t* tcp = f.data() + 34;
    uint16_t sp = htons(443), dp = htons(49152);
    std::memcpy(tcp, &sp, 2);
    std::memcpy(tcp + 2, &dp, 2);
    uint32_t sn = htonl(seq);
    std::memcpy(tcp + 4, &sn, 4);
    uint32_t an = htonl(1);
    std::memcpy(tcp + 8, &an, 4);
    tcp[12] = 0x50;
    tcp[13] = 0x18;
    uint16_t win = htons(65535);
    std::memcpy(tcp + 14, &win, 2);

    uint8_t* pl = tcp + TCP_HDR;
    for (size_t i = 0; i < payload_sz; ++i)
        pl[i] = static_cast<uint8_t>(i);
    return f;
}

static std::vector<uint8_t> build_icmp_echo(uint8_t type, uint8_t code,
                                             uint16_t id, uint16_t seq,
                                             size_t payload_sz, uint16_t ip_id) {
    const size_t ICMP_HDR = 8;
    std::vector<uint8_t> f(34 + ICMP_HDR + payload_sz, 0);
    fill_eth_ip(f.data(), 1, ip_id, static_cast<uint16_t>(ICMP_HDR + payload_sz));

    uint8_t* icmp = f.data() + 34;
    icmp[0] = type;
    icmp[1] = code;
    uint16_t id_n = htons(id);
    std::memcpy(icmp + 4, &id_n, 2);
    uint16_t sq = htons(seq);
    std::memcpy(icmp + 6, &sq, 2);
    for (size_t i = 0; i < payload_sz; ++i)
        icmp[ICMP_HDR + i] = static_cast<uint8_t>(i);

    uint16_t ck = ip_checksum(icmp, ICMP_HDR + payload_sz);
    std::memcpy(icmp + 2, &ck, 2);
    return f;
}

static std::vector<uint8_t> build_icmp_error(uint8_t type, uint8_t code,
                                              uint16_t ip_id) {
    const size_t ICMP_HDR = 8;
    const size_t ORIG = 28;   // original IP header (20) + 8 bytes of original datagram
    std::vector<uint8_t> f(34 + ICMP_HDR + ORIG, 0);
    fill_eth_ip(f.data(), 1, ip_id, static_cast<uint16_t>(ICMP_HDR + ORIG));

    uint8_t* icmp = f.data() + 34;
    icmp[0] = type;
    icmp[1] = code;

    uint8_t* orig = icmp + ICMP_HDR;
    orig[0] = 0x45;
    orig[8] = 64;
    orig[9] = 6;   // original was TCP
    uint32_t osrc = htonl(0x0A006401);
    uint32_t odst = htonl(0xCB007164);
    std::memcpy(orig + 12, &osrc, 4);
    std::memcpy(orig + 16, &odst, 4);

    uint16_t ck = ip_checksum(icmp, ICMP_HDR + ORIG);
    std::memcpy(icmp + 2, &ck, 2);
    return f;
}

// ---- Command handlers ----

static void do_ping(int cfd, nicsim::TapDevice& tap) {
    const int COUNT = 10;
    const size_t PAYLOAD = 56;

    std::ostringstream oss;
    oss << "PING 203.0.113.100 via " << tap.dev_name()
        << ": " << PAYLOAD << " data bytes\n";
    respond(cfd, oss.str());

    uint16_t ip_id = 1;
    for (int i = 1; i <= COUNT && !g_exit_flag; ++i) {
        auto frame = build_icmp_echo(0, 0, 0x1234,
                                     static_cast<uint16_t>(i), PAYLOAD, ip_id++);
        auto t0 = std::chrono::steady_clock::now();
        tap.inject(frame.data(), frame.size());
        auto t1 = std::chrono::steady_clock::now();
        double ms = std::chrono::duration<double, std::milli>(t1 - t0).count();

        oss.str("");
        oss << static_cast<int>(34 + 8 + PAYLOAD)
            << " bytes from 203.0.113.100: icmp_seq=" << i
            << " ttl=64 time=" << std::fixed << std::setprecision(1) << ms << " ms\n";
        respond(cfd, oss.str());

        if (i < COUNT)
            std::this_thread::sleep_for(std::chrono::seconds(1));
    }

    oss.str("");
    oss << "\n--- 203.0.113.100 ping statistics ---\n"
        << COUNT << " packets transmitted, " << COUNT
        << " received, 0% packet loss\n";
    respond(cfd, oss.str());
}

static void do_icmp(int cfd, nicsim::TapDevice& tap) {
    respond(cfd, "Injecting ICMP traffic on " + tap.dev_name() + " ...\n");

    struct Spec { uint8_t type; uint8_t code; const char* name; bool error; };
    Spec specs[] = {
        { 8, 0, "Echo Request",               false },
        { 8, 0, "Echo Request",               false },
        { 8, 0, "Echo Request",               false },
        { 0, 0, "Echo Reply",                 false },
        { 0, 0, "Echo Reply",                 false },
        { 0, 0, "Echo Reply",                 false },
        { 3, 1, "Dest Unreachable (host)",     true },
        { 3, 3, "Dest Unreachable (port)",     true },
        {11, 0, "Time Exceeded",               true },
    };

    uint16_t ip_id = 1, seq = 1;
    uint64_t total_bytes = 0;
    int count = 0;

    for (auto& s : specs) {
        std::vector<uint8_t> frame;
        if (s.error)
            frame = build_icmp_error(s.type, s.code, ip_id++);
        else
            frame = build_icmp_echo(s.type, s.code, 0x1234, seq++, 56, ip_id++);

        tap.inject(frame.data(), frame.size());
        total_bytes += frame.size();
        count++;

        std::ostringstream oss;
        oss << "  " << std::left << std::setw(30) << s.name
            << "(type=" << std::setw(2) << static_cast<int>(s.type)
            << " code=" << static_cast<int>(s.code) << ")  -> "
            << frame.size() << " bytes\n";
        respond(cfd, oss.str());
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
    }

    std::ostringstream oss;
    oss << "\n--- " << count << " ICMP packets injected, "
        << total_bytes << " bytes total ---\n";
    respond(cfd, oss.str());
}

static void do_download(int cfd, nicsim::TapDevice& tap, uint64_t total_size) {
    const size_t PAYLOAD  = 1400;
    const size_t FRAME_SZ = 34 + 20 + PAYLOAD;          // 1454
    const double TARGET   = 10e6;                         // 10 MB/s
    const size_t FPS      = static_cast<size_t>(TARGET / FRAME_SZ);
    const size_t BATCH    = FPS / 20;

    uint64_t size_mb = total_size / (1024 * 1024);
    double est = static_cast<double>(total_size) / TARGET;

    std::ostringstream oss;
    oss << "Downloading " << size_mb << " MB at ~10 MB/s  (est. "
        << static_cast<int>(est) << "s) ...\n";
    respond(cfd, oss.str());

    uint32_t tcp_seq = 1;
    uint16_t ip_id   = 1;
    uint64_t sent    = 0, frames = 0;

    auto frame   = build_tcp_frame(tcp_seq, ip_id, PAYLOAD);
    auto t_start = std::chrono::steady_clock::now();
    auto t_rpt   = t_start;
    uint64_t rpt_bytes = 0;

    while (sent < total_size && !g_exit_flag) {
        for (size_t i = 0; i < BATCH && sent < total_size; ++i) {
            uint32_t sn = htonl(tcp_seq);
            std::memcpy(frame.data() + 38, &sn, 4);
            uint16_t idn = htons(ip_id);
            std::memcpy(frame.data() + 18, &idn, 2);
            std::memset(frame.data() + 24, 0, 2);
            uint16_t ck = ip_checksum(frame.data() + 14, 20);
            std::memcpy(frame.data() + 24, &ck, 2);

            ssize_t w = tap.inject(frame.data(), frame.size());
            if (w > 0) {
                sent += static_cast<uint64_t>(w);
                frames++;
                tcp_seq += static_cast<uint32_t>(PAYLOAD);
                ip_id++;
            }
        }

        auto now = std::chrono::steady_clock::now();
        double since = std::chrono::duration<double>(now - t_rpt).count();
        if (since >= 1.0) {
            double elapsed = std::chrono::duration<double>(now - t_start).count();
            double speed   = static_cast<double>(sent - rpt_bytes) / since / 1e6;
            double pct     = 100.0 * static_cast<double>(sent)
                                   / static_cast<double>(total_size);

            oss.str("");
            oss << "  [" << std::setw(4) << static_cast<int>(elapsed) << "s]  "
                << std::setw(7) << (sent / 1000000) << " / "
                << (total_size / 1000000) << " MB  ("
                << std::fixed << std::setprecision(0) << pct << "%)  "
                << std::setprecision(1) << speed << " MB/s\n";
            respond(cfd, oss.str());
            t_rpt = now;
            rpt_bytes = sent;
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }

    auto t_end = std::chrono::steady_clock::now();
    double dur = std::chrono::duration<double>(t_end - t_start).count();

    oss.str("");
    oss << "\n--- Download Complete ---\n"
        << "  Size     : " << (sent / 1000000) << " MB\n"
        << "  Frames   : " << frames << "\n"
        << "  Duration : " << std::fixed << std::setprecision(1) << dur << " s\n"
        << "  Avg speed: " << (static_cast<double>(sent) / 1e6 / dur) << " MB/s\n";
    respond(cfd, oss.str());
}

// ---- Server loop ----

static std::string read_command(int fd) {
    char buf[1024];
    ssize_t n = ::recv(fd, buf, sizeof(buf) - 1, 0);
    if (n <= 0) return "";
    buf[n] = '\0';
    std::string cmd(buf);
    while (!cmd.empty() && (cmd.back() == '\n' || cmd.back() == '\r'))
        cmd.pop_back();
    return cmd;
}

static void run_server() {
    std::signal(SIGINT,  on_signal);
    std::signal(SIGTERM, on_signal);
    std::signal(SIGPIPE, SIG_IGN);

    std::cout << "=== nicsim TAP Server ===\n\n";

    nicsim::TapDevice tap("nicsim0");
    if (!tap.create()) {
        std::cerr << "Failed to create TAP device.  Run with:  sudo ./nicsim\n";
        return;
    }
    if (!tap.configure("10.0.100.1")) {
        std::cerr << "Failed to configure interface\n";
        return;
    }
    if (!tap.bring_up()) {
        std::cerr << "Failed to bring interface up\n";
        return;
    }

    std::cout << "Interface '" << tap.dev_name() << "' is UP  (10.0.100.1/24)\n";
    std::cout << "Verify with:  ifconfig " << tap.dev_name() << "\n\n";

    ::unlink(SOCKET_PATH);
    int srv_fd = ::socket(AF_UNIX, SOCK_STREAM, 0);
    if (srv_fd < 0) { std::cerr << "socket() failed\n"; return; }

    struct sockaddr_un addr{};
    addr.sun_family = AF_UNIX;
    std::strncpy(addr.sun_path, SOCKET_PATH, sizeof(addr.sun_path) - 1);

    if (::bind(srv_fd, reinterpret_cast<struct sockaddr*>(&addr), sizeof(addr)) < 0) {
        std::cerr << "bind() failed\n"; ::close(srv_fd); return;
    }
    if (::listen(srv_fd, 5) < 0) {
        std::cerr << "listen() failed\n"; ::close(srv_fd); return;
    }

    std::cout << "Listening on " << SOCKET_PATH << "\n"
              << "Commands:  test_nicsim --ping | --icmp | --download --1gb | --exit\n"
              << "Press Ctrl+C to stop.\n\n";

    struct pollfd pfd{};
    pfd.fd = srv_fd;
    pfd.events = POLLIN;

    while (!g_exit_flag) {
        int ret = ::poll(&pfd, 1, 1000);
        if (ret <= 0) continue;

        int cfd = ::accept(srv_fd, nullptr, nullptr);
        if (cfd < 0) continue;

        std::string cmd = read_command(cfd);
        if (cmd.empty()) { ::close(cfd); continue; }

        std::cout << "[cmd] " << cmd << "\n";

        if (cmd == "PING") {
            do_ping(cfd, tap);
        } else if (cmd == "ICMP") {
            do_icmp(cfd, tap);
        } else if (cmd.rfind("DOWNLOAD ", 0) == 0) {
            uint64_t sz = std::stoull(cmd.substr(9));
            do_download(cfd, tap, sz);
        } else if (cmd == "EXIT") {
            respond(cfd, "Shutting down nicsim server ...\n");
            g_exit_flag = 1;
        } else {
            respond(cfd, "Unknown command: " + cmd + "\n");
        }

        ::close(cfd);
    }

    ::close(srv_fd);
    ::unlink(SOCKET_PATH);
    std::cout << "\nServer stopped.  Interface '" << tap.dev_name() << "' removed.\n";
}

#endif  // __linux__

int main() {
#ifdef __linux__
    run_server();
#else
    demo_two_nics();
    demo_packet_overflow();
    demo_multi_segment();
    std::cout << "\nNote: TAP interface features require Linux.\n";
#endif
    return 0;
}
