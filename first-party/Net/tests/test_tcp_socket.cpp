#include "test_common.h"

#include <cstring>
#include <thread>

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

    printf("OK\n");
}
