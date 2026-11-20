#pragma once

#include <memory>

#include "Address.h"
#include "SocketError.h"

namespace Net {
    class SocketContext;

    class UDPSocket {
        std::shared_ptr<SocketContext> context_;
        SocketHandle handle_ = INVALID_SOCK;
        Address local_addr_;
        mutable int last_error_ = 0;
    public:
        UDPSocket();

        ~UDPSocket();

        [[nodiscard]] bool IsOpen() const { return sock_is_open(handle_); }

        [[nodiscard]] Address local_addr() const { return local_addr_; }

        [[nodiscard]] int last_error() const { return last_error_; }

        void Open(unsigned short port, bool reuse_addr = true);

        void Close();

        bool Send(const Address &destination, const void *data, int size) const;

        // >0 bytes read; 0 = zero-length datagram; -1 = no data pending (would-block); -2 = error, see last_error()
        int Receive(Address &sender, void *data, int size) const;

        bool JoinMulticast(const Address &addr);
        bool DropMulticast(const Address &addr);

        bool SetBlocking(bool is_blocking) const;
    };

    class TCPSocket {
        std::shared_ptr<SocketContext> context_;
        SocketHandle handle_ = INVALID_SOCK, connection_ = INVALID_SOCK;
        Address remote_addr_;
        int last_error_ = 0;
    public:
        TCPSocket();

        TCPSocket(TCPSocket &&rhs) noexcept;

        ~TCPSocket();

        TCPSocket(const TCPSocket &rhs) = delete;

        TCPSocket &operator=(const TCPSocket &rhs) = delete;

        TCPSocket &operator=(TCPSocket &&rhs) noexcept;

        static TCPSocket PassClientConnection(TCPSocket &rhs) {
            TCPSocket ret;
            ret.handle_ = INVALID_SOCK;
            ret.connection_ = rhs.connection_;
            ret.remote_addr_ = rhs.remote_addr_;
            rhs.Listen();
            return ret;
        }

        [[nodiscard]] bool IsOpen() const { return sock_is_open(handle_); }

        [[nodiscard]] bool connected() const { return sock_is_open(connection_); }

        [[nodiscard]] Address remote_addr() const { return remote_addr_; }

        [[nodiscard]] int last_error() const { return last_error_; }

        void Open(unsigned short port, bool reuse_addr = true);

        void Close();

        void CloseClient();

        bool Listen();

        bool Accept(bool is_blocking = false);

        bool Connect(const Address &dest);

        bool Send(const void *data, int size);

        // >0 bytes read; 0 = peer closed the connection; -1 = no data pending (would-block); -2 = error, see last_error()
        int Receive(void *data, int size);

        bool SetBlocking(bool is_blocking);

        void WaitClientComplete(int t_ms);
    };

    bool SetBlocking(SocketHandle sock, bool is_blocking);
}
