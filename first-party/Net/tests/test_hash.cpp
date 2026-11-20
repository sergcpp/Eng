#include "test_common.h"

#include <string>

#include "../hash/Crc32.h"

void test_hash() {
    printf("Test hash               | ");

    { // crc32_fast must agree with every reference variant (Eng mixes them)
        std::string big;
        for (int i = 0; i < 4000; ++i) {
            big += char('A' + i % 26);
        }
        const std::string inputs[] = {"", "a", "abc", "123456789", big};
        for (const auto &s : inputs) {
            const uint32_t fast = crc32_fast(s.data(), s.size());
            require(fast == crc32_bitwise(s.data(), s.size()));
            require(fast == crc32_halfbyte(s.data(), s.size()));
            require(fast == crc32_1byte(s.data(), s.size()));
            require(fast == crc32_4bytes(s.data(), s.size()));
            require(fast == crc32_8bytes(s.data(), s.size()));
            require(fast == crc32_16bytes(s.data(), s.size()));
        }
    }

    printf("OK\n");
}
