#pragma once

#include "GL.h"

void APIENTRY ren_glCreateTextures_emu(GLenum target, GLsizei n, GLuint *textures);

void APIENTRY ren_glTextureStorage2D_Comp_emu(GLenum target, GLuint texture, GLsizei levels, GLenum internalformat,
                                              GLsizei width, GLsizei height);
void APIENTRY ren_glTextureStorage3D_Comp_emu(GLenum target, GLuint texture, GLsizei levels, GLenum internalformat,
                                              GLsizei width, GLsizei height, GLsizei depth);

void APIENTRY ren_glTextureSubImage2D_Comp_emu(GLenum target, GLuint texture, GLint level, GLint xoffset, GLint yoffset,
                                               GLsizei width, GLsizei height, GLenum format, GLenum type,
                                               const void *pixels);
void APIENTRY ren_glTextureSubImage3D_Comp_emu(GLenum target, GLuint texture, GLint level, GLint xoffset, GLint yoffset,
                                               GLint zoffset, GLsizei width, GLsizei height, GLsizei depth,
                                               GLenum format, GLenum type, const void *pixels);

void APIENTRY ren_glCompressedTextureSubImage2D_Comp_emu(GLenum target, GLuint texture, GLint level, GLint xoffset,
                                                         GLint yoffset, GLsizei width, GLsizei height, GLenum format,
                                                         GLsizei imageSize, const void *data);
void APIENTRY ren_glCompressedTextureSubImage3D_Comp_emu(GLenum target, GLuint texture, GLint level, GLint xoffset,
                                                         GLint yoffset, GLint zoffset, GLsizei width, GLsizei height,
                                                         GLsizei depth, GLenum format, GLsizei imageSize,
                                                         const void *data);

void APIENTRY ren_glTextureParameterf_Comp_emu(GLenum target, GLuint texture, GLenum pname, GLfloat param);
void APIENTRY ren_glTextureParameteri_Comp_emu(GLenum target, GLuint texture, GLenum pname, GLint param);

void APIENTRY ren_glTextureParameterfv_Comp_emu(GLenum target, GLuint texture, GLenum pname, const GLfloat *params);
void APIENTRY ren_glTextureParameteriv_Comp_emu(GLenum target, GLuint texture, GLenum pname, const GLint *params);

void APIENTRY ren_glGenerateTextureMipmap_Comp_emu(GLenum target, GLuint texture);

void APIENTRY ren_glBindTextureUnit_Comp_emu(GLenum target, GLuint unit, GLuint texture);
#ifndef __ANDROID__
void APIENTRY ren_glNamedBufferStorage_Comp_emu(GLenum target, GLuint buffer, GLsizeiptr size, const void *data,
                                                GLbitfield flags);
#endif