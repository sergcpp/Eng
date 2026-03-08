#pragma once

#if defined(REN_GL_BACKEND)
#include "Gl/SamplerGL.h"
#elif defined(REN_VK_BACKEND)
#include "Vk/SamplerVK.h"
#endif
