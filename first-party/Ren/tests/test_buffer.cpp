#include "test_common.h"

#include "membuf.h"

#include "../Buffer.h"
#include "../Context.h"
#include "../Utils.h"

void test_buffer() {
    using namespace Ren;

    printf("Test buffer             | ");

    { // Test suballocation
        TestContext test;

        BufferMain buf_main = {};
        BufferCold buf_cold = {};
        require(Buffer_Init(test.api(), buf_main, buf_cold, String{"buf"}, eBufType::Uniform, 256, test.log()));

        const SubAllocation a1 = Buffer_AllocSubRegion(test.api(), buf_main, buf_cold, 16, 1, "temp", test.log());
        require(a1.offset == 0);
        require(Buffer_AllocSubRegion(test.api(), buf_main, buf_cold, 32, 1, "temp", test.log()).offset == 16);
        require(Buffer_AllocSubRegion(test.api(), buf_main, buf_cold, 64, 1, "temp", test.log()).offset == 16 + 32);
        require(Buffer_AllocSubRegion(test.api(), buf_main, buf_cold, 16, 1, "temp", test.log()).offset ==
                16 + 32 + 64);
        require(Buffer_AllocSubRegion(test.api(), buf_main, buf_cold, 256 - (16 + 32 + 64 + 16), 1, "temp", test.log())
                    .offset == 16 + 32 + 64 + 16);

        require(Buffer_FreeSubRegion(buf_cold, a1));
        require(Buffer_AllocSubRegion(test.api(), buf_main, buf_cold, 16, 1, "temp", test.log()).offset == 0);

        Buffer_Destroy(test.api(), buf_main, buf_cold);
    }

    printf("OK\n");
}
