#pragma once

#if defined(REN_VK_BACKEND)
#include "Vk/ProgramVK.h"
#elif defined(REN_GL_BACKEND)
#include "Gl/ProgramGL.h"
#endif
