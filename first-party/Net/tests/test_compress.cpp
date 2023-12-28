#include "test_common.h"

#include <cstdio>

#include "../Compress.h"

void test_compress() {
    Net::Packet test_buf(1024);
    for (int i = 0; i < 256; i++) {
        test_buf[i] = (uint8_t)(i % 255);
    }

    for (int i = 256; i < 768; i++) {
        test_buf[i] = 0;
    }

    for (int i = 768; i < (int)test_buf.size(); i++) {
        test_buf[i] = (uint8_t)(i % 255);
    }

    Net::Packet compr = Net::CompressLZO(test_buf);

    printf("size before %i, size after %i\n", int(test_buf.size()), int(compr.size()));

    Net::Packet decompr = Net::DecompressLZO(compr);

    assert(decompr.size() == test_buf.size());
    assert(test_buf == decompr);
}