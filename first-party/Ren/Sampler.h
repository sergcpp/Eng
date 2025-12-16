#pragma once

#if defined(REN_GL_BACKEND)
#include "SamplerGL.h"
#elif defined(REN_VK_BACKEND)
#include "SamplerVK.h"
#endif
