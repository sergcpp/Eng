cmake_minimum_required(VERSION 3.10)
project(test_SW)

add_executable(test_SW main.c
                       test_buffer.c
                       test_common.h
                       test_context.c
                       test_framebuffer.c
                       test_pixels.c
                       test_program.c
                       test_texture.c
                       test_zbuffer.c)
target_link_libraries(test_SW ${LIBS} SW)

set_target_properties(test_SW PROPERTIES OUTPUT_NAME_DEBUG test_SW-dbg)
set_target_properties(test_SW PROPERTIES OUTPUT_NAME_RELWITHDEBINFO test_SW-dev)
set_target_properties(test_SW PROPERTIES OUTPUT_NAME_ASAN test_SW-asan)
set_target_properties(test_SW PROPERTIES OUTPUT_NAME_RELEASE test_SW)