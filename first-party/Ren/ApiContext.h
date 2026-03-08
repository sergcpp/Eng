#pragma once

#if defined(REN_VK_BACKEND)
#include "Vk/VKCtx.h"
#elif defined(REN_GL_BACKEND)
#include "Gl/GLCtx.h"
#endif