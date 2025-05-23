cmake_minimum_required(VERSION 3.10)
project(Phy)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
add_definitions(-D_DISABLE_CONSTEXPR_MUTEX_CONSTRUCTOR)

set(CMAKE_CONFIGURATION_TYPES "Debug;Release;RelWithDebInfo;Asan" CACHE STRING "" FORCE)

IF(MSVC)
    if (NOT DEFINED CMAKE_CXX_FLAGS_ASAN OR CMAKE_CXX_FLAGS_ASAN STREQUAL "")
        set(CMAKE_CXX_FLAGS_ASAN "/MTd /Zi /Od /RTC1 /fsanitize=address /analyze /WX")
        set(CMAKE_EXE_LINKER_FLAGS_ASAN "${CMAKE_EXE_LINKER_FLAGS_DEBUG} /INCREMENTAL:NO")
    endif()
ELSE(MSVC)
ENDIF(MSVC)

IF(WIN32)

ELSE(WIN32)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pthread -fPIC")
    if(NOT CMAKE_SYSTEM_NAME MATCHES "Android")
        set(LIBS dl pthread)
    endif()
ENDIF(WIN32)

set(SOURCE_FILES    AlignedAlloc.h
                    Body.h
                    Body.cpp
                    Bounds.h
                    BVHSplit.h
                    BVHSplit.cpp
                    Core.h
                    MMat.h
                    MQuat.h
                    MVec.h
                    Phy.h
                    Phy.cpp
                    Shape.h
                    Shape.cpp
                    SmallVector.h
                    Span.h
                    Utils.h
                    Utils.cpp)

list(APPEND PHY_SOURCE_FILES ${SOURCE_FILES})
source_group("src" FILES ${SOURCE_FILES})

if(MSVC)
    list(APPEND PHY_SOURCE_FILES _CustomTypes.natvis)
endif(MSVC)

add_library(Phy STATIC ${PHY_SOURCE_FILES})
target_link_libraries(Phy ${LIBS})

add_subdirectory(tests)