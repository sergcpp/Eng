#include "test_common.h"

#include <atomic>
#include <cstring>
#include <thread>
#include <vector>

#ifdef _WIN32
#include <winsock2.h>
#include <ws2ipdef.h>
#include <ws2tcpip.h>
#else
#include <netinet/tcp.h>
#endif

#include "../Socket.h"

void test_tcp_socket() {
    using namespace Net;

    printf("Test tcp_socket         | ");

    { // TCPSocket open/close
        TCPSocket socket;
        require(!socket.IsOpen());
        require_nothrow(socket.Open(0));
        require(socket.IsOpen());
        require(socket.local_addr().port() != 0);
        socket.Close();
        require(!socket.IsOpen());
        require_nothrow(socket.Open(30000));
        require(socket.local_addr().port() == 30000);
        require(socket.IsOpen());
    }
    { // TCPSocket bind address is reported back
        TCPSocket s;
        require_nothrow(s.Open(Address(127, 0, 0, 1, 0), 0));
        require(s.local_addr().a() == 127);
        require(s.local_addr().port() != 0);
    }
    { // TCPSocket same port fail
        TCPSocket a, b;
        require_nothrow(a.Open(30000, false));
        require_throws(b.Open(30000, false));
        require(a.IsOpen());
        require(!b.IsOpen());
    }
    { // connect to a port with no listener must fail, not report success
        TCPSocket probe;
        require_nothrow(probe.Open(Address(127, 0, 0, 1, 0), 0));
        const unsigned short dead_port = probe.local_addr().port();
        probe.Close();

        TCPSocket a;
        require_nothrow(a.Open(Address(127, 0, 0, 1, 0), 0));
        require(!a.Connect(Address(127, 0, 0, 1, dead_port)));
        require(a.last_error() != 0);
    }
    { // fresh connection with no data pending: -1 (would-block), not 0
        TCPSocket server, client;
        require_nothrow(server.Open(Address(127, 0, 0, 1, 0), 0));
        require(server.Listen());
        require_nothrow(client.Open(Address(127, 0, 0, 1, 0), 0));

        std::thread thr([&]() {
            for (int i = 0; i < 100 && !server.Accept(); ++i) {
                std::this_thread::sleep_for(std::chrono::milliseconds(50));
            }
        });
        require(client.Connect(Address(127, 0, 0, 1, server.local_addr().port()), 2000));
        thr.join();
        require(server.connected());

        char buffer[64];
        require(client.Receive(buffer, sizeof(buffer)) == -1);
    }
    { // WaitReadable: false on an idle socket, true once data arrives
        TCPSocket server, client;
        require_nothrow(server.Open(Address(127, 0, 0, 1, 0), 0));
        require(server.Listen());
        require_nothrow(client.Open(Address(127, 0, 0, 1, 0), 0));

        std::thread thr([&]() {
            for (int i = 0; i < 100 && !server.Accept(); ++i) {
                std::this_thread::sleep_for(std::chrono::milliseconds(50));
            }
            // delay the send so the idle-socket check below cannot race with it
            std::this_thread::sleep_for(std::chrono::milliseconds(300));
            const char data[] = "x";
            for (int i = 0; i < 100 && !server.Send(data, sizeof(data)); ++i) {
                std::this_thread::sleep_for(std::chrono::milliseconds(50));
            }
        });
        require(client.Connect(Address(127, 0, 0, 1, server.local_addr().port()), 2000));

        require(!client.WaitReadable(50));
        require(client.WaitReadable(2000));
        thr.join();
    }
    { // TCP_NODELAY is enabled after Accept/Connect
        TCPSocket server, client;
        require_nothrow(server.Open(Address(127, 0, 0, 1, 0), 0));
        require(server.Listen());
        require_nothrow(client.Open(Address(127, 0, 0, 1, 0), 0));

        std::thread thr([&]() {
            for (int i = 0; i < 100 && !server.Accept(); ++i) {
                std::this_thread::sleep_for(std::chrono::milliseconds(50));
            }
        });
        require(client.Connect(Address(127, 0, 0, 1, server.local_addr().port()), 2000));
        thr.join();
        require(server.connected());

        int nodelay = 0;
        socklen_t len = sizeof(nodelay);
        require(getsockopt(client.socket_handle(), IPPROTO_TCP, TCP_NODELAY, (char *) &nodelay, &len) == 0);
        require(nodelay != 0);
        nodelay = 0;
        require(getsockopt(server.connection_handle(), IPPROTO_TCP, TCP_NODELAY, (char *) &nodelay, &len) == 0);
        require(nodelay != 0);
    }
    { // TCPSocket send and receive packets
        TCPSocket server, client;
        require_nothrow(server.Open(Address(127, 0, 0, 1, 0), 0));
        require(server.Listen());
        require_nothrow(client.Open(Address(127, 0, 0, 1, 0), 0));

        const char packet[] = "packet data";
        bool echoed = false;
        std::thread thr([&]() {
            for (int i = 0; i < 100 && !server.Accept(); ++i) {
                std::this_thread::sleep_for(std::chrono::milliseconds(50));
            }
            for (int i = 0; i < 100 && !echoed; ++i) {
                char buffer[256];
                const int bytes_read = server.Receive(buffer, sizeof(buffer));
                if (bytes_read > 0) {
                    echoed = bytes_read == sizeof(packet) && strcmp(buffer, packet) == 0 &&
                             server.Send(packet, sizeof(packet));
                }
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
            }
        });
        require(client.Connect(Address(127, 0, 0, 1, server.local_addr().port()), 2000));

        bool client_received = false;
        for (int i = 0; i < 100 && !(client_received && echoed); ++i) {
            require(client.Send(packet, sizeof(packet)));

            char buffer[256];
            const int bytes_read = client.Receive(buffer, sizeof(buffer));
            if (bytes_read > 0) {
                client_received = bytes_read == sizeof(packet) && strcmp(buffer, packet) == 0;
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
        thr.join();
        require(echoed);
        require(client_received);
    }
    { // SendAll of 1 MiB through a small send buffer arrives complete and in order
        TCPSocket server, client;
        require_nothrow(server.Open(Address(127, 0, 0, 1, 0), 0));
        require(server.Listen());
        require_nothrow(client.Open(Address(127, 0, 0, 1, 0), 0));

        const int sndbuf = 8192;
        require(setsockopt(client.socket_handle(), SOL_SOCKET, SO_SNDBUF, (char *) &sndbuf, sizeof(sndbuf)) == 0);

        const size_t payload_size = 1u << 20;
        std::vector<unsigned char> payload(payload_size);
        unsigned int rnd = 0x12345u;
        for (auto &b : payload) {
            rnd = rnd * 1103515245u + 12345u;
            b = (unsigned char) (rnd >> 16u);
        }
        // order-sensitive FNV-1a over the whole stream
        unsigned int expected_hash = 2166136261u;
        for (unsigned char b : payload) {
            expected_hash = (expected_hash ^ b) * 16777619u;
        }

        std::atomic<size_t> total{0};
        unsigned int received_hash = 2166136261u;
        std::thread thr([&]() {
            for (int i = 0; i < 100 && !server.Accept(); ++i) {
                std::this_thread::sleep_for(std::chrono::milliseconds(50));
            }
            char buffer[8192];
            int reads = 0;
            while (total < payload_size && server.WaitReadable(2000)) {
                const int bytes_read = server.Receive(buffer, sizeof(buffer));
                if (bytes_read > 0) {
                    for (int i = 0; i < bytes_read; ++i) {
                        received_hash = (received_hash ^ (unsigned char) buffer[i]) * 16777619u;
                    }
                    total += (size_t) bytes_read;
                    // throttle the reader so the small send buffer actually fills up
                    if (++reads % 16 == 0) {
                        std::this_thread::sleep_for(std::chrono::milliseconds(1));
                    }
                } else if (bytes_read == 0 || bytes_read == -2) {
                    break;
                }
            }
        });
        require(client.Connect(Address(127, 0, 0, 1, server.local_addr().port()), 2000));

        require(client.SendAll(payload.data(), payload_size, 15000));
        thr.join();
        require(total == payload_size);
        require(received_hash == expected_hash);
    }
    { // RecvExactly reassembles a stream that arrives in single-byte pieces
        TCPSocket server, client;
        require_nothrow(server.Open(Address(127, 0, 0, 1, 0), 0));
        require(server.Listen());
        require_nothrow(client.Open(Address(127, 0, 0, 1, 0), 0));

        const size_t payload_size = 4096;
        std::vector<unsigned char> payload(payload_size);
        for (size_t i = 0; i < payload_size; ++i) {
            payload[i] = (unsigned char) (i * 7u + 3u);
        }

        std::thread thr([&]() {
            for (int i = 0; i < 100 && !server.Accept(); ++i) {
                std::this_thread::sleep_for(std::chrono::milliseconds(50));
            }
            for (size_t i = 0; i < payload_size; ++i) {
                for (int r = 0; r < 200 && !server.Send(payload.data() + i, 1); ++r) {
                    std::this_thread::sleep_for(std::chrono::milliseconds(25));
                }
            }
        });
        require(client.Connect(Address(127, 0, 0, 1, server.local_addr().port()), 2000));

        std::vector<unsigned char> received(payload_size, 0);
        require(client.RecvExactly(received.data(), payload_size, 10000));
        thr.join();
        require(memcmp(received.data(), payload.data(), payload_size) == 0);
    }
    { // orderly close: peer Shutdown + Close reads as Receive() == 0
        TCPSocket server, client;
        require_nothrow(server.Open(Address(127, 0, 0, 1, 0), 0));
        require(server.Listen());
        require_nothrow(client.Open(Address(127, 0, 0, 1, 0), 0));

        std::thread thr([&]() {
            for (int i = 0; i < 100 && !server.Accept(); ++i) {
                std::this_thread::sleep_for(std::chrono::milliseconds(50));
            }
            server.Shutdown();
            server.Close();
        });
        require(client.Connect(Address(127, 0, 0, 1, server.local_addr().port()), 2000));
        thr.join();

        require(client.WaitReadable(2000));
        char buffer[16];
        require(client.Receive(buffer, sizeof(buffer)) == 0);
        require(client.Receive(buffer, sizeof(buffer)) == 0);
        require(!client.RecvExactly(buffer, sizeof(buffer), 500));
    }
    { // peer fully closed: writes eventually fail with an error, not silently succeed
        TCPSocket server, client;
        require_nothrow(server.Open(Address(127, 0, 0, 1, 0), 0));
        require(server.Listen());
        require_nothrow(client.Open(Address(127, 0, 0, 1, 0), 0));

        std::thread thr([&]() {
            for (int i = 0; i < 100 && !server.Accept(); ++i) {
                std::this_thread::sleep_for(std::chrono::milliseconds(50));
            }
            server.Shutdown();
            server.Close();
            server.CloseClient();
        });
        require(client.Connect(Address(127, 0, 0, 1, server.local_addr().port()), 2000));
        thr.join();

        const char chunk[] = "ping";
        bool send_failed = false;
        for (int i = 0; i < 100 && !send_failed; ++i) {
            send_failed = !client.Send(chunk, sizeof(chunk));
            if (!send_failed) {
                std::this_thread::sleep_for(std::chrono::milliseconds(50));
            }
        }
        require(send_failed);
        require(client.last_error() != 0);
    }

    printf("OK\n");
}
