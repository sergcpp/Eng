#pragma warning(disable : 4996)

#include "Socket.h"

#include <cassert>
#include <cerrno>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <stdexcept>

#ifdef _WIN32
#include <winsock2.h>
#include <ws2ipdef.h>
#include <Ws2tcpip.h>
#endif
#if defined(__linux__) || defined(__APPLE__)

#include <fcntl.h>
#include <netdb.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/time.h>

#endif

class Net::SocketContext {
public:
    SocketContext() {   // NOLINT
        //srand((unsigned int)time(nullptr));
#ifdef _WIN32
        WSADATA WsaData;
        const int res = WSAStartup(MAKEWORD(2, 2), &WsaData);
        if (res != 0) {
            throw std::runtime_error("WSAStartup failed!");
        }
#endif
    }

    ~SocketContext() {  // NOLINT
#ifdef _WIN32
        WSACleanup();
#endif
    }
};

#ifdef __ANDROID__
extern unsigned int g_android_local_ip_address;
#endif

namespace {
    std::weak_ptr<Net::SocketContext> shrd_context;

    unsigned int GetLocalAddr() {
        unsigned int local_addr = 0;
#ifndef __ANDROID__
        char hostname[256];
        if (gethostname(hostname, sizeof(hostname)) != -1) {
            struct hostent *phe = gethostbyname(hostname);
            int i = 0;
            while (phe != nullptr && phe->h_addr_list[i] != nullptr) {
                struct in_addr addr;    // NOLINT
                memcpy(&addr, phe->h_addr_list[i++], sizeof(struct in_addr));
                local_addr = ntohl(addr.s_addr);
                auto a = (unsigned char) (local_addr >> 24u);
                if (a == 192 || a == 10 || a == 172) {
                    break;
                }
            }
        }
#else
        local_addr = ntohl(g_android_local_ip_address);
#endif
        return local_addr;
    }
}

Net::UDPSocket::UDPSocket() {
    if (shrd_context.expired()) {
        shrd_context = context_ = std::make_shared<SocketContext>();
    } else {
        context_ = shrd_context.lock();
    }
}

Net::UDPSocket::~UDPSocket() {
    Close();
}

void Net::UDPSocket::Open(unsigned short port, bool reuse_addr) {
    handle_ = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (!sock_is_open(handle_)) {
        handle_ = INVALID_SOCK;
        throw std::runtime_error("Cannot create socket.");
    }

    if (reuse_addr) {
        int one = 1;
        setsockopt(handle_, SOL_SOCKET, SO_REUSEADDR, (char *) &one, sizeof(int));
    }

    sockaddr_in address;    // NOLINT
    memset(&address, 0, sizeof(address));
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;

    address.sin_port = htons(port);

    if (::bind(handle_, (const sockaddr *) &address, sizeof(sockaddr_in)) < 0) {
        Close();
        throw std::runtime_error("Cannot bind socket.");
    }

    unsigned int local_addr = GetLocalAddr();

    struct sockaddr_in sin; // NOLINT
    socklen_t addrlen = sizeof(sin);
    if (getsockname(handle_, (struct sockaddr *) &sin, &addrlen) == 0 &&
        sin.sin_family == AF_INET &&
        addrlen == sizeof(sin)) {
        local_addr_ = Address(local_addr, ntohs(sin.sin_port));
    }

    SetBlocking(false);
}

void Net::UDPSocket::Close() {
    if (sock_is_open(handle_)) {
#ifdef _WIN32
        closesocket(handle_);
#else
        close(handle_);
#endif
        handle_ = INVALID_SOCK;
    }
}

bool Net::UDPSocket::Send(const Address &destination, const void *data, int size) const {
    assert(data);
    assert(size > 0);

    if (!sock_is_open(handle_)) {
        return false;
    }

    sockaddr_in address;    // NOLINT
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = htonl(destination.address());
    address.sin_port = htons((unsigned short) destination.port());

    const int sent_bytes = int(sendto(handle_, (const char *) data, size, 0, (sockaddr *) &address, sizeof(sockaddr_in)));
    if (sent_bytes != size) {
        last_error_ = last_socket_error();
        return false;
    }

    return true;
}

int Net::UDPSocket::Receive(Address &sender, void *data, const int size) const {
    assert(data);
    assert(size > 0);

    if (!sock_is_open(handle_)) {
        last_error_ = 0;
        return -1;
    }

    sockaddr_in from;   // NOLINT
    socklen_t from_len = sizeof(sockaddr_in);

    const int received_bytes = int(recvfrom(handle_, (char *) data, size, 0, (sockaddr *) &from, &from_len));

    if (received_bytes < 0) {
        last_error_ = last_socket_error();
        return sock_would_block(last_error_) || last_error_ == intr_error() ? -1 : -2;
    }

    last_error_ = 0;

    unsigned int address = ntohl(from.sin_addr.s_addr);
    unsigned short port = ntohs(from.sin_port);

    sender = Address(address, port);

    return received_bytes;
}

bool Net::UDPSocket::JoinMulticast(const Address &addr) {
    struct ip_mreq mreq;    // NOLINT
    mreq.imr_interface.s_addr = htonl(local_addr_.address());
    mreq.imr_multiaddr.s_addr = htonl(addr.address());
    return setsockopt(handle_, IPPROTO_IP, IP_ADD_MEMBERSHIP, (const char *) &mreq, sizeof(mreq)) == 0;
}

bool Net::UDPSocket::DropMulticast(const Address &addr) {
    struct ip_mreq mreq;    // NOLINT
    mreq.imr_interface.s_addr = htonl(local_addr_.address());
    mreq.imr_multiaddr.s_addr = htonl(addr.address());
    return setsockopt(handle_, IPPROTO_IP, IP_DROP_MEMBERSHIP, (const char *) &mreq, sizeof(mreq)) == 0;
}

bool Net::UDPSocket::SetBlocking(bool is_blocking) const {
    return Net::SetBlocking(handle_, is_blocking);
}

Net::TCPSocket::TCPSocket() {
    if (shrd_context.expired()) {
        shrd_context = context_ = std::make_shared<SocketContext>();
    } else {
        context_ = shrd_context.lock();
    }
}

Net::TCPSocket::TCPSocket(TCPSocket &&rhs) noexcept
        : context_(std::move(rhs.context_)), handle_(rhs.handle_), connection_(rhs.connection_),
          remote_addr_(rhs.remote_addr_) {
    rhs.handle_ = INVALID_SOCK;
    rhs.connection_ = INVALID_SOCK;
    rhs.remote_addr_ = Address();
}

Net::TCPSocket::~TCPSocket() {
    Close();
    CloseClient();
}

Net::TCPSocket &Net::TCPSocket::operator=(TCPSocket &&rhs) noexcept {
    context_ = std::move(rhs.context_);
    handle_ = rhs.handle_;
    connection_ = rhs.connection_;
    remote_addr_ = rhs.remote_addr_;

    rhs.handle_ = INVALID_SOCK;
    rhs.connection_ = INVALID_SOCK;
    rhs.remote_addr_ = Address();
    return *this;
}

void Net::TCPSocket::Open(unsigned short port, bool reuse_addr) {
    handle_ = socket(AF_INET, SOCK_STREAM, 0);
    if (!sock_is_open(handle_)) {
        handle_ = INVALID_SOCK;
        throw std::runtime_error("Cannot create socket.");
    }

    if (reuse_addr) {
        int one = 1;
        setsockopt(handle_, SOL_SOCKET, SO_REUSEADDR, (char *) &one, sizeof(int));
    }

    sockaddr_in address;    // NOLINT
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(port);

    if (::bind(handle_, (const sockaddr *) &address, sizeof(sockaddr_in)) < 0) {
        Close();
        throw std::runtime_error("Cannot bind socket.");
    }

    SetBlocking(false);
}

void Net::TCPSocket::Close() {
    if (sock_is_open(handle_)) {
#ifdef _WIN32
        closesocket(handle_);
#else
        close(handle_);
#endif
        handle_ = INVALID_SOCK;
    }
}

void Net::TCPSocket::CloseClient() {
    if (sock_is_open(connection_)) {
#ifdef _WIN32
        closesocket(connection_);
#else
        close(connection_);
#endif
        connection_ = INVALID_SOCK;
    }
}

bool Net::TCPSocket::Listen() {
    if (!sock_is_open(handle_)) {
        return false;
    }
    connection_ = INVALID_SOCK;
    return listen(handle_, 1024) == 0;
}

bool Net::TCPSocket::Accept(bool is_blocking) {
    if (!sock_is_open(handle_)) {
        return false;
    }

    sockaddr_in from;   // NOLINT
    socklen_t from_len = sizeof(from);

    connection_ = accept(handle_, (struct sockaddr *) &from, &from_len);
    if (!sock_is_open(connection_)) {
        last_error_ = last_socket_error();
        connection_ = INVALID_SOCK;
        return false;
    }

    Net::SetBlocking(connection_, is_blocking);

    unsigned int address = ntohl(from.sin_addr.s_addr);
    unsigned int port = ntohs(from.sin_port);
    remote_addr_ = Address(address, port);

    return true;
}

bool Net::TCPSocket::Connect(const Address &dest) {
    if (!sock_is_open(handle_)) {
        return false;
    }

    sockaddr_in address;    // NOLINT
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = htonl(dest.address());
    address.sin_port = htons((unsigned short) dest.port());

    [[maybe_unused]] int res = connect(handle_, (struct sockaddr *)&address, sizeof(sockaddr_in));

    connection_ = INVALID_SOCK;
    return true;
}

bool Net::TCPSocket::Send(const void *data, int size) {
    assert(data);
    assert(size > 0);

    const SocketHandle dst = sock_is_open(connection_) ? connection_ : handle_;
    if (!sock_is_open(dst)) {
        return false;
    }

    const int sent_bytes = int(send(dst, (char *) data, (size_t) size, 0));
    if (sent_bytes != size) {
        last_error_ = last_socket_error();
        return false;
    }

    return true;
}

int Net::TCPSocket::Receive(void *data, int size) {
    assert(data);
    assert(size > 0);

    const SocketHandle src = sock_is_open(connection_) ? connection_ : handle_;
    if (!sock_is_open(src)) {
        last_error_ = 0;
        return -2;
    }

    const int received_bytes = int(recv(src, (char *) data, (size_t) size, 0));

    if (received_bytes == 0) {
        return 0; // orderly peer shutdown
    }
    if (received_bytes < 0) {
        last_error_ = last_socket_error();
        return sock_would_block(last_error_) || last_error_ == intr_error() ? -1 : -2;
    }

    last_error_ = 0;
    return received_bytes;
}

bool Net::TCPSocket::SetBlocking(bool is_blocking) {
    return Net::SetBlocking(handle_, is_blocking);
}

void Net::TCPSocket::WaitClientComplete(int t_ms) {
    /*int valopt;
    fd_set myset;
    struct timeval tv;
    socklen_t lon;

    int ss = connection_ ? connection_ : handle_;

    tv.tv_sec = 0;
    tv.tv_usec = 1000 * t_ms;
    FD_ZERO(&myset);
    FD_SET(ss, &myset);
    if (select(ss + 1, NULL, &myset, NULL, &tv) > 0) {
        lon = sizeof(int);
        getsockopt(ss, SOL_SOCKET, SO_ERROR, (char *)(&valopt), &lon);
        if (valopt) {
            printf("Error in connection() %d - %s\n", valopt, strerror(valopt));
            return;
        }
    } else {
        printf("Timeout or error()\n");
        return;
    }*/
}

bool Net::SetBlocking(SocketHandle sock, bool is_blocking) {
    bool ret = true;
#ifdef _WIN32
    /// @note windows sockets are created in blocking mode by default
    // currently on windows, there is no easy way to obtain the socket's current blocking mode since WSAIsBlocking was deprecated
    u_long non_blocking = is_blocking ? 0 : 1;
    ret = NO_ERROR == ioctlsocket(sock, FIONBIO, &non_blocking);
#else
    const int flags = fcntl(sock, F_GETFL, 0);
    if ((flags & O_NONBLOCK) && !is_blocking) { return ret; }
    if (!(flags & O_NONBLOCK) && is_blocking) { return ret; }
    ret = (0 == fcntl(sock, F_SETFL, is_blocking ? flags ^ O_NONBLOCK : flags | O_NONBLOCK));
#endif
    return ret;
}
