#ifndef SHADOW_INTERFACE_H
#define SHADOW_INTERFACE_H

#include "_interface_common.h"

INTERFACE_START(Shadow)

struct Params {
    mat4 g_shadow_view_proj_mat;
};

const int U_M_MATRIX_LOC = 12;

INTERFACE_END

#endif // SHADOW_INTERFACE_H