#pragma once

namespace Ren {
    struct CpuFeatures {
        unsigned sse2_supported : 1;
        unsigned sse3_supported : 1;
        unsigned ssse3_supported : 1;
        unsigned sse41_supported : 1;
        unsigned avx_supported : 1;
        unsigned avx2_supported : 1;
    };

    extern CpuFeatures g_CpuFeatures;
}
