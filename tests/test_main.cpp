#include "nicsim/mac_address.h"
#include "nicsim/ethernet_frame.h"
#include "nicsim/packet_buffer.h"
#include "nicsim/nic_device.h"
#include "nicsim/network_medium.h"
#include <cassert>
#include <iostream>

using namespace nicsim;

static int tests_run = 0;
static int tests_passed = 0;

#define TEST(name)                                          \
    do {                                                    \
        ++tests_run;                                        \
        std::cout << "  " << #name << "... ";               \
    } while (0)

#define PASS()                                              \
    do {                                                    \
        ++tests_passed;                                     \
        std::cout << "PASS\n";                              \
    } while (0)

static void test_mac_address() {
    TEST(mac_parse_and_format);
    MacAddress m(MacAddress::Bytes{0xDE, 0xAD, 0xBE, 0xEF, 0x00, 0x01});
    assert(m.to_string() == "de:ad:be:ef:00:01");
    PASS();

    TEST(mac_from_string);
    MacAddress m2("aa:bb:cc:dd:ee:ff");
    assert(m2.to_string() == "aa:bb:cc:dd:ee:ff");
    PASS();

    TEST(mac_broadcast);
    auto bc = MacAddress::broadcast();
    assert(bc.is_broadcast());
    assert(bc.is_multicast());
    PASS();

    TEST(mac_random_is_local);
    auto r = MacAddress::random();
    assert((r.bytes()[0] & 0x02) != 0);   // locally administered
    assert((r.bytes()[0] & 0x01) == 0);    // unicast
    PASS();
}

static void test_ethernet_frame() {
    TEST(frame_serialize_deserialize);
    EthernetFrame frame;
    frame.destination = MacAddress(MacAddress::Bytes{0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF});
    frame.source      = MacAddress(MacAddress::Bytes{0x02, 0x00, 0x00, 0x00, 0x00, 0x01});
    frame.ether_type  = EtherType::ARP;
    frame.payload     = {1, 2, 3, 4, 5};

    auto bytes = frame.serialize();
    auto frame2 = EthernetFrame::deserialize(bytes);

    assert(frame2.destination == frame.destination);
    assert(frame2.source == frame.source);
    assert(frame2.ether_type == frame.ether_type);
    assert(frame2.payload == frame.payload);
    PASS();

    TEST(frame_crc_deterministic);
    assert(frame.compute_crc() == frame.compute_crc());
    PASS();
}

static void test_packet_buffer() {
    TEST(buffer_enqueue_dequeue);
    PacketBuffer buf(2);
    EthernetFrame f;
    f.payload = {0xAA};

    assert(buf.enqueue(f));
    assert(buf.enqueue(f));
    assert(!buf.enqueue(f));   // full
    assert(buf.dropped() == 1);
    assert(buf.size() == 2);

    auto out = buf.dequeue();
    assert(out.has_value());
    assert(buf.size() == 1);
    PASS();
}

static void test_nic_device() {
    TEST(nic_transmit_requires_link_up);
    NicDevice nic;
    EthernetFrame f;
    f.destination = MacAddress::broadcast();
    f.payload = {0x01};
    assert(!nic.transmit(std::move(f)));   // link is down
    assert(nic.stats().tx_errors.load() == 1);
    PASS();

    TEST(nic_end_to_end);
    NicConfig cfg_a, cfg_b;
    cfg_a.name = "a";
    cfg_b.name = "b";
    NicDevice a(cfg_a), b(cfg_b);
    NetworkMedium medium;
    medium.connect(&a);
    medium.connect(&b);
    a.link_up();
    b.link_up();

    EthernetFrame f2;
    f2.destination = b.mac();
    f2.ether_type  = EtherType::IPv4;
    f2.payload.assign(64, 0x42);
    assert(a.transmit(std::move(f2)));

    auto rx = b.poll_rx();
    assert(rx.has_value());
    assert(rx->source == a.mac());
    assert(rx->payload.size() == 64);
    PASS();

    TEST(nic_rejects_wrong_destination);
    EthernetFrame f3;
    f3.destination = MacAddress::random();  // some other MAC
    f3.payload.assign(64, 0x00);
    a.transmit(std::move(f3));
    auto rx2 = b.poll_rx();
    assert(!rx2.has_value());
    PASS();

    TEST(nic_promiscuous_accepts_all);
    b.set_promiscuous(true);
    EthernetFrame f4;
    f4.destination = MacAddress::random();
    f4.payload.assign(64, 0xFF);
    a.transmit(std::move(f4));
    auto rx3 = b.poll_rx();
    assert(rx3.has_value());
    PASS();
}

int main() {
    std::cout << "Running nicsim tests...\n";

    test_mac_address();
    test_ethernet_frame();
    test_packet_buffer();
    test_nic_device();

    std::cout << "\n" << tests_passed << "/" << tests_run << " tests passed.\n";
    return (tests_passed == tests_run) ? 0 : 1;
}
