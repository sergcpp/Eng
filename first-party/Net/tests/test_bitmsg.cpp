#include "test_common.h"

#include <cstring>

#include "../BitMsg.h"

class TestMsg : public Net::BitMsg {
    uint8_t buf[256];

  public:
    TestMsg() : BitMsg(buf, sizeof(buf)) { memset(buf, 0, sizeof(buf)); }

    size_t &len() { return len_; }
    uint8_t *write_data() { return write_data_; }
};

void test_bitmsg() {
    printf("Test bitmsg             | ");

    { // Wrong bits write number should throw runtime_error
        TestMsg msg;
        require_throws(msg.WriteBits(0, 0));
        require_throws(msg.WriteBits(0, -32));
        require_throws(msg.WriteBits(0, 33));
        // Overflow check
        require_throws(msg.WriteBits(16, 3));
        require_nothrow(msg.WriteBits(7, 3));
        require_throws(msg.WriteBits(-7, 3));
        require_throws(msg.WriteBits(4, -3));
        require_nothrow(msg.WriteBits(3, -3));
        require_throws(msg.WriteBits(-10, -3));
        require_nothrow(msg.WriteBits(-4, -3));
    }
    { // Write values
        TestMsg msg;
        require(msg.len() == 0);
        msg.WriteBits(1, 1);
        require(msg.write_data()[0] == 0b1);
        require(msg.len() == 0);
        msg.WriteBits(4, 3);
        require(msg.write_data()[0] == 0b1001);
        require(msg.len() == 0);
        msg.WriteBits(2, 2);
        require(msg.write_data()[0] == 0b101001);
        require(msg.len() == 0);
        msg.WriteBits(3, 2);
        require(msg.write_data()[0] == 0b11101001);
        require(msg.len() == 1);
        msg.WriteBits(1, 1);
        require(msg.write_data()[0] == 0b11101001);
        require(msg.write_data()[1] == 0b1);
        require(msg.len() == 1);
        msg.WriteBits(12, 4);
        require(msg.write_data()[0] == 0b11101001);
        require(msg.write_data()[1] == 0b11001);
        require(msg.len() == 1);
    }
    { // Wrong bits read number should throw runtime_error
        TestMsg msg;
        require_throws(msg.ReadBits(0));
        require_throws(msg.ReadBits(-32));
        require_throws(msg.ReadBits(33));
    }
    { // Read values
        TestMsg msg;
        msg.len() = 2;
        msg.write_data()[0] = 0b11101001;
        msg.write_data()[1] = 0b11001;
        require(msg.ReadBits(1) == 1);
        require(msg.ReadBits(3) == 4);
        require(msg.ReadBits(2) == 2);
        require(msg.ReadBits(2) == 3);
        require(msg.ReadBits(1) == 1);
        require(msg.ReadBits(4) == 12);
    }
    { // Writing values
        TestMsg msg;

        bool b = true;
        char c1 = -123;
        uint8_t c2 = 253;
        short s1 = -12345;
        unsigned short s2 = 29103;
        int i1 = -891355;
        unsigned i2 = 6565558;
        int64_t ll1 = 656555812345558;
        int temp = 121213345;
        float f1 = *reinterpret_cast<float *>(&temp);
        msg.Write(b);
        require(msg.write_data()[0] == 0b1);
        msg.Write<int8_t>(c1);
        require(msg.write_data()[0] == 0b00001011);
        require(msg.write_data()[1] == 0b1);
        msg.Write(c2);
        require(msg.write_data()[0] == 0b00001011);
        require(msg.write_data()[1] == 0b11111011);
        require(msg.write_data()[2] == 0b1);
        msg.Write(s1);
        require(msg.write_data()[0] == 0b00001011);
        require(msg.write_data()[1] == 0b11111011);
        require(msg.write_data()[2] == 0b10001111);
        require(msg.write_data()[3] == 0b10011111);
        require(msg.write_data()[4] == 0b1);
        msg.Write(s2);
        require(msg.write_data()[0] == 0b00001011);
        require(msg.write_data()[1] == 0b11111011);
        require(msg.write_data()[2] == 0b10001111);
        require(msg.write_data()[3] == 0b10011111);
        require(msg.write_data()[4] == 0b01011111);
        require(msg.write_data()[5] == 0b11100011);
        require(msg.write_data()[6] == 0b0);
        msg.Write<int32_t>(i1);
        require(msg.write_data()[0] == 0b00001011);
        require(msg.write_data()[1] == 0b11111011);
        require(msg.write_data()[2] == 0b10001111);
        require(msg.write_data()[3] == 0b10011111);
        require(msg.write_data()[4] == 0b01011111);
        require(msg.write_data()[5] == 0b11100011);
        require(msg.write_data()[6] == 0b01001010);
        require(msg.write_data()[7] == 0b11001100);
        require(msg.write_data()[8] == 0b11100100);
        require(msg.write_data()[9] == 0b11111111);
        require(msg.write_data()[10] == 0b1);
        msg.Write<uint32_t>(i2);
        require(msg.write_data()[0] == 0b00001011);
        require(msg.write_data()[1] == 0b11111011);
        require(msg.write_data()[2] == 0b10001111);
        require(msg.write_data()[3] == 0b10011111);
        require(msg.write_data()[4] == 0b01011111);
        require(msg.write_data()[5] == 0b11100011);
        require(msg.write_data()[6] == 0b01001010);
        require(msg.write_data()[7] == 0b11001100);
        require(msg.write_data()[8] == 0b11100100);
        require(msg.write_data()[9] == 0b11111111);
        require(msg.write_data()[10] == 0b01101101);
        require(msg.write_data()[11] == 0b01011101);
        require(msg.write_data()[12] == 0b11001000);
        require(msg.write_data()[13] == 0b00000000);
        require(msg.write_data()[14] == 0b0);
        msg.Write<int64_t>(ll1);
        require(msg.write_data()[0] == 0b00001011);
        require(msg.write_data()[1] == 0b11111011);
        require(msg.write_data()[2] == 0b10001111);
        require(msg.write_data()[3] == 0b10011111);
        require(msg.write_data()[4] == 0b01011111);
        require(msg.write_data()[5] == 0b11100011);
        require(msg.write_data()[6] == 0b01001010);
        require(msg.write_data()[7] == 0b11001100);
        require(msg.write_data()[8] == 0b11100100);
        require(msg.write_data()[9] == 0b11111111);
        require(msg.write_data()[10] == 0b01101101);
        require(msg.write_data()[11] == 0b01011101);
        require(msg.write_data()[12] == 0b11001000);
        require(msg.write_data()[13] == 0b00000000);
        require(msg.write_data()[14] == 0b10101100);
        require(msg.write_data()[15] == 0b10101101);
        require(msg.write_data()[16] == 0b11110000);
        require(msg.write_data()[17] == 0b10011111);
        require(msg.write_data()[18] == 0b01000100);
        require(msg.write_data()[19] == 0b10101010);
        require(msg.write_data()[20] == 0b00000100);
        require(msg.write_data()[21] == 0b00000000);
        require(msg.write_data()[22] == 0b0);
        msg.Write<float>(f1);
        require(msg.write_data()[0] == 0b00001011);
        require(msg.write_data()[1] == 0b11111011);
        require(msg.write_data()[2] == 0b10001111);
        require(msg.write_data()[3] == 0b10011111);
        require(msg.write_data()[4] == 0b01011111);
        require(msg.write_data()[5] == 0b11100011);
        require(msg.write_data()[6] == 0b01001010);
        require(msg.write_data()[7] == 0b11001100);
        require(msg.write_data()[8] == 0b11100100);
        require(msg.write_data()[9] == 0b11111111);
        require(msg.write_data()[10] == 0b01101101);
        require(msg.write_data()[11] == 0b01011101);
        require(msg.write_data()[12] == 0b11001000);
        require(msg.write_data()[13] == 0b00000000);
        require(msg.write_data()[14] == 0b10101100);
        require(msg.write_data()[15] == 0b10101101);
        require(msg.write_data()[16] == 0b11110000);
        require(msg.write_data()[17] == 0b10011111);
        require(msg.write_data()[18] == 0b01000100);
        require(msg.write_data()[19] == 0b10101010);
        require(msg.write_data()[20] == 0b00000100);
        require(msg.write_data()[21] == 0b00000000);
        require(msg.write_data()[22] == 0b01000010);
        require(msg.write_data()[23] == 0b00100011);
        require(msg.write_data()[24] == 0b01110011);
        require(msg.write_data()[25] == 0b00001110);
        require(msg.write_data()[26] == 0b0);
    }
    { // Reading values
        TestMsg msg;

        msg.len() = 27;
        msg.write_data()[0] = 0b00001011;
        msg.write_data()[1] = 0b11111011;
        msg.write_data()[2] = 0b10001111;
        msg.write_data()[3] = 0b10011111;
        msg.write_data()[4] = 0b01011111;
        msg.write_data()[5] = 0b11100011;
        msg.write_data()[6] = 0b01001010;
        msg.write_data()[7] = 0b11001100;
        msg.write_data()[8] = 0b11100100;
        msg.write_data()[9] = 0b11111111;
        msg.write_data()[10] = 0b01101101;
        msg.write_data()[11] = 0b01011101;
        msg.write_data()[12] = 0b11001000;
        msg.write_data()[13] = 0b00000000;
        msg.write_data()[14] = 0b10101100;
        msg.write_data()[15] = 0b10101101;
        msg.write_data()[16] = 0b11110000;
        msg.write_data()[17] = 0b10011111;
        msg.write_data()[18] = 0b01000100;
        msg.write_data()[19] = 0b10101010;
        msg.write_data()[20] = 0b00000100;
        msg.write_data()[21] = 0b00000000;
        msg.write_data()[22] = 0b01000010;
        msg.write_data()[23] = 0b00100011;
        msg.write_data()[24] = 0b01110011;
        msg.write_data()[25] = 0b00001110;
        msg.write_data()[26] = 0b0;
        require(msg.Read<bool>());
        require(msg.Read<int8_t>() == -123);
        require(msg.Read<uint8_t>() == 253);
        require(msg.Read<int16_t>() == -12345);
        require(msg.Read<uint16_t>() == 29103);
        require(msg.Read<int32_t>() == -891355);
        require(msg.Read<uint32_t>() == 6565558);
        require(msg.Read<int64_t>() == 656555812345558);
        int temp = 121213345;
        float f1 = *reinterpret_cast<float *>(&temp);
        require(msg.Read<float>() == f1);
    }

    printf("OK\n");
}