#ifndef BLIT_SSR_DILATE_INTERFACE_H
#define BLIT_SSR_DILATE_INTERFACE_H

#include "_interface_common.h"

INTERFACE_START(SSRDilate)

struct Params {
    vec4 transform;
};

const int SSR_TEX_SLOT = 0;

INTERFACE_END

#endif // BLIT_SSR_DILATE_INTERFACE_H