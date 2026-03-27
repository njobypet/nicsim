#include <cstdlib>
#include <cstring>
#include <iostream>
#include <string>
#include <strings.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

static constexpr const char* SOCK_PATH = "/tmp/nicsim.sock";

static void usage() {
    std::cerr
        << "Usage: test_nicsim <command> [options]\n\n"
        << "Commands:\n"
        << "  --ping                Simulate ping (10 ICMP echo replies)\n"
        << "  --icmp                Inject various ICMP packet types\n"
        << "  --download --<size>   Simulate file download\n"
        << "                        sizes: --1gb  --500mb  --100mb  --10mb\n"
        << "  --exit                Shut down the nicsim server\n\n"
        << "The nicsim server must be running (sudo ./nicsim).\n";
}

static uint64_t parse_size(const char* arg) {
    const char* p = arg;
    while (*p == '-') p++;

    char* end = nullptr;
    unsigned long num = std::strtoul(p, &end, 10);
    if (end == p || num == 0) return 0;

    if (strncasecmp(end, "gb", 2) == 0) return num * 1024ULL * 1024 * 1024;
    if (strncasecmp(end, "mb", 2) == 0) return num * 1024ULL * 1024;
    if (strncasecmp(end, "kb", 2) == 0) return num * 1024ULL;
    return num;
}

static int connect_to_server() {
    int fd = ::socket(AF_UNIX, SOCK_STREAM, 0);
    if (fd < 0) return -1;

    struct sockaddr_un addr{};
    addr.sun_family = AF_UNIX;
    std::strncpy(addr.sun_path, SOCK_PATH, sizeof(addr.sun_path) - 1);

    if (::connect(fd, reinterpret_cast<struct sockaddr*>(&addr), sizeof(addr)) < 0) {
        ::close(fd);
        return -1;
    }
    return fd;
}

static void send_and_stream(int fd, const std::string& cmd) {
    std::string msg = cmd + "\n";
    ::send(fd, msg.c_str(), msg.size(), 0);

    char buf[4096];
    ssize_t n;
    while ((n = ::read(fd, buf, sizeof(buf) - 1)) > 0) {
        buf[n] = '\0';
        std::cout << buf << std::flush;
    }
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        usage();
        return 1;
    }

    std::string command;

    if (std::strcmp(argv[1], "--ping") == 0) {
        command = "PING";
    } else if (std::strcmp(argv[1], "--icmp") == 0) {
        command = "ICMP";
    } else if (std::strcmp(argv[1], "--download") == 0) {
        if (argc < 3) {
            std::cerr << "Error: --download requires a size (e.g. --1gb, --500mb)\n";
            return 1;
        }
        uint64_t size = parse_size(argv[2]);
        if (size == 0) {
            std::cerr << "Error: invalid size '" << argv[2] << "'\n";
            return 1;
        }
        command = "DOWNLOAD " + std::to_string(size);
    } else if (std::strcmp(argv[1], "--exit") == 0) {
        command = "EXIT";
    } else {
        std::cerr << "Unknown command: " << argv[1] << "\n\n";
        usage();
        return 1;
    }

    int fd = connect_to_server();
    if (fd < 0) {
        std::cerr << "Cannot connect to nicsim server.\n"
                  << "Is it running?  Start with:  sudo ./nicsim\n";
        return 1;
    }

    send_and_stream(fd, command);
    ::close(fd);
    return 0;
}
