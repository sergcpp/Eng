#include "UDPConnection.h"

#include <cassert>
#include <cstring>

#include "hash/Crc32.h"

Net::UDPConnection::UDPConnection(unsigned int protocol_id, float timeout_s)
    : protocol_id_(protocol_id), timeout_s_(timeout_s), running_(false), mode_(eMode::None) {
    ClearData();
}

Net::UDPConnection::~UDPConnection() {
    if (running_) {
        Stop();
    }
}

void Net::UDPConnection::Start(int port) {
    assert(!running_);
    socket_.Open(port);
    running_ = true;
    OnStart();
}

void Net::UDPConnection::Stop() {
    assert(running_);
    bool conn = connected();
    ClearData();
    socket_.Close();
    running_ = false;
    if (conn) {
        OnDisconnect();
    }
    OnStop();
}

void Net::UDPConnection::Listen() {
    bool conn = connected();
    ClearData();
    if (conn) {
        OnDisconnect();
    }
    mode_ = eMode::Server;
    state_ = eState::Listening;
}

void Net::UDPConnection::Connect(const Address &address) {
    printf("client connecting to %d.%d.%d.%d:%d\n", address.a(), address.b(), address.c(), address.d(), address.port());
    bool conn = connected();
    ClearData();
    if (conn) {
        OnDisconnect();
    }
    mode_ = eMode::Client;
    state_ = eState::Connecting;
    address_ = address;
}

void Net::UDPConnection::Update(float dt_s) {
    assert(running_);
    timeout_acc_ += dt_s;
    if (timeout_acc_ > timeout_s_) {
        if (state_ == eState::Connecting) {
            printf("connect timed out\n");
            ClearData();
            state_ = eState::ConnectFailed;
            OnDisconnect();
        } else if (state_ == eState::Connected) {
            printf("connection timed out\n");
            ClearData();
            if (state_ == eState::Connecting) {
                state_ = eState::ConnectFailed;
            }
            OnDisconnect();
        }
    }
}

bool Net::UDPConnection::SendPacket(const unsigned char data[], int size) {
    assert(running_);
    assert(MAX_PACKET_SIZE >= size + 4);
    if (address_.address() == 0) {
        return false;
    }

    unsigned char packet[MAX_PACKET_SIZE];
    packet[0] = (unsigned char)(protocol_id_ >> 24u);
    packet[1] = (unsigned char)((protocol_id_ >> 16u) & 0xFFu);
    packet[2] = (unsigned char)((protocol_id_ >> 8u) & 0xFFu);
    packet[3] = (unsigned char)((protocol_id_)&0xFFu);
    memcpy(&packet[4], data, size);

    { // compute crc32 and use it instead protocol id
        uint32_t crc = crc32_fast(packet, size + 4);
        packet[0] = (unsigned char)(crc >> 24u);
        packet[1] = (unsigned char)((crc >> 16u) & 0xFFu);
        packet[2] = (unsigned char)((crc >> 8u) & 0xFFu);
        packet[3] = (unsigned char)((crc)&0xFFu);
    }

    return socket_.Send(address_, packet, size + 4);
}

int Net::UDPConnection::ReceivePacket(unsigned char data[], int size) {
    Address sender;
    assert(running_);

    assert(size + 4 <= MAX_PACKET_SIZE);
    unsigned char packet[MAX_PACKET_SIZE];
    int bytes_read = socket_.Receive(sender, packet, size + 4);
    if (bytes_read <= 4) {
        return 0;
    }

    { // check protocol id hashsum
        uint32_t crc = (((unsigned int)packet[0] << 24u) | ((unsigned int)packet[1] << 16u) |
                        ((unsigned int)packet[2] << 8u) | ((unsigned int)packet[3]));

        packet[0] = (unsigned char)(protocol_id_ >> 24u);
        packet[1] = (unsigned char)((protocol_id_ >> 16u) & 0xFFu);
        packet[2] = (unsigned char)((protocol_id_ >> 8u) & 0xFFu);
        packet[3] = (unsigned char)((protocol_id_)&0xFFu);

        uint32_t crc_check = crc32_fast(packet, bytes_read);

        if (crc != crc_check) {
            return 0;
        }
    }

    if (mode_ == eMode::Server && !connected()) {
        state_ = eState::Connected;
        address_ = sender;
        OnConnect();
    }
    if (sender == address_) {
        if (mode_ == eMode::Client && state_ == eState::Connecting) {
            state_ = eState::Connected;
            OnConnect();
        }
        timeout_acc_ = 0;

        memcpy(data, &packet[4], bytes_read - 4);
        return bytes_read - 4;
    }
    return 0;
}