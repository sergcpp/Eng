// https://registry.khronos.org/OpenGL/specs/gl/GLSLangSpec.4.60.html#keywords
DECORATE(const,                 false /* is_typename */, false /* reserved */)
DECORATE(uniform,               false,  false)
DECORATE(buffer,                false,  false)
DECORATE(shared,                false,  false)
DECORATE(attribute,             false,  false)
DECORATE(varying,               false,  false)
DECORATE(coherent,              false,  false)
DECORATE(volatile,              false,  false)
DECORATE(restrict,              false,  false)
DECORATE(readonly,              false,  false)
DECORATE(writeonly,             false,  false)
DECORATE(atomic_uint,           true,   false)
DECORATE(layout,                false,  false)
DECORATE(centroid,              false,  false)
DECORATE(flat,                  false,  false)
DECORATE(smooth,                false,  false)
DECORATE(noperspective,         false,  false)
DECORATE(patch,                 false,  false)
DECORATE(sample,                false,  false)
DECORATE(invariant,             false,  false)
DECORATE(precise,               false,  false)
DECORATE(break,                 false,  false)
DECORATE(continue,              false,  false)
DECORATE(do,                    false,  false)
DECORATE(for,                   false,  false)
DECORATE(while,                 false,  false)
DECORATE(switch,                false,  false)
DECORATE(case,                  false,  false)
DECORATE(default,               false,  false)
DECORATE(if,                    false,  false)
DECORATE(else,                  false,  false)
DECORATE(subroutine,            false,  false)
DECORATE(in,                    false,  false)
DECORATE(out,                   false,  false)
DECORATE(inout,                 false,  false)
DECORATE(int,                   true,   false)
DECORATE(void,                  true,   false)
DECORATE(bool,                  true,   false)
DECORATE(true,                  false,  false)
DECORATE(false,                 false,  false)
DECORATE(float,                 true,   false)
DECORATE(double,                true,   false)
DECORATE(discard,               false,  false)
DECORATE(return,                false,  false)
DECORATE(vec2,                  true,   false)
DECORATE(vec3,                  true,   false)
DECORATE(vec4,                  true,   false)
DECORATE(ivec2,                 true,   false)
DECORATE(ivec3,                 true,   false)
DECORATE(ivec4,                 true,   false)
DECORATE(bvec2,                 true,   false)
DECORATE(bvec3,                 true,   false)
DECORATE(bvec4,                 true,   false)
DECORATE(uint,                  true,   false)
DECORATE(uvec2,                 true,   false)
DECORATE(uvec3,                 true,   false)
DECORATE(uvec4,                 true,   false)
DECORATE(dvec2,                 true,   false)
DECORATE(dvec3,                 true,   false)
DECORATE(dvec4,                 true,   false)
DECORATE(mat2,                  true,   false)
DECORATE(mat3,                  true,   false)
DECORATE(mat4,                  true,   false)
DECORATE(mat2x2,                true,   false)
DECORATE(mat2x3,                true,   false)
DECORATE(mat2x4,                true,   false)
DECORATE(mat3x2,                true,   false)
DECORATE(mat3x3,                true,   false)
DECORATE(mat3x4,                true,   false)
DECORATE(mat4x2,                true,   false)
DECORATE(mat4x3,                true,   false)
DECORATE(mat4x4,                true,   false)
DECORATE(dmat2,                 true,   false)
DECORATE(dmat3,                 true,   false)
DECORATE(dmat4,                 true,   false)
DECORATE(dmat2x2,               true,   false)
DECORATE(dmat2x3,               true,   false)
DECORATE(dmat2x4,               true,   false)
DECORATE(dmat3x2,               true,   false)
DECORATE(dmat3x3,               true,   false)
DECORATE(dmat3x4,               true,   false)
DECORATE(dmat4x2,               true,   false)
DECORATE(dmat4x3,               true,   false)
DECORATE(dmat4x4,               true,   false)
DECORATE(lowp,                  false,  false)
DECORATE(mediump,               false,  false)
DECORATE(highp,                 false,  false)
DECORATE(precision,             false,  false)
DECORATE(sampler1D,             true,   false)
DECORATE(sampler1DShadow,       true,   false)
DECORATE(sampler1DArray,        true,   false)
DECORATE(sampler1DArrayShadow,  true,   false)
DECORATE(isampler1D,            true,   false)
DECORATE(isampler1DArray,       true,   false)
DECORATE(usampler1D,            true,   false)
DECORATE(usampler1DArray,       true,   false)
DECORATE(sampler2D,             true,   false)
DECORATE(sampler2DShadow,       true,   false)
DECORATE(sampler2DArray,        true,   false)
DECORATE(sampler2DArrayShadow,  true,   false)
DECORATE(isampler2D,            true,   false)
DECORATE(isampler2DArray,       true,   false)
DECORATE(usampler2D,            true,   false)
DECORATE(usampler2DArray,       true,   false)
DECORATE(sampler2DRect,         true,   false)
DECORATE(sampler2DRectShadow,   true,   false)
DECORATE(isampler2DRect,        true,   false)
DECORATE(usampler2DRect,        true,   false)
DECORATE(sampler2DMS,           true,   false)
DECORATE(isampler2DMS,          true,   false)
DECORATE(usampler2DMS,          true,   false)
DECORATE(sampler2DMSArray,      true,   false)
DECORATE(isampler2DMSArray,     true,   false)
DECORATE(usampler2DMSArray,     true,   false)
DECORATE(sampler3D,             true,   false)
DECORATE(isampler3D,            true,   false)
DECORATE(usampler3D,            true,   false)
DECORATE(samplerCube,           true,   false)
DECORATE(samplerCubeShadow,     true,   false)
DECORATE(isamplerCube,          true,   false)
DECORATE(usamplerCube,          true,   false)
DECORATE(samplerCubeArray,      true,   false)
DECORATE(samplerCubeArrayShadow, true,  false)
DECORATE(isamplerCubeArray,     true,   false)
DECORATE(usamplerCubeArray,     true,   false)
DECORATE(samplerBuffer,         true,   false)
DECORATE(isamplerBuffer,        true,   false)
DECORATE(usamplerBuffer,        true,   false)
DECORATE(image1D,               true,   false)
DECORATE(iimage1D,              true,   false)
DECORATE(uimage1D,              true,   false)
DECORATE(image1DArray,          true,   false)
DECORATE(iimage1DArray,         true,   false)
DECORATE(uimage1DArray,         true,   false)
DECORATE(image2D,               true,   false)
DECORATE(iimage2D,              true,   false)
DECORATE(uimage2D,              true,   false)
DECORATE(image2DArray,          true,   false)
DECORATE(iimage2DArray,         true,   false)
DECORATE(uimage2DArray,         true,   false)
DECORATE(image2DRect,           true,   false)
DECORATE(iimage2DRect,          true,   false)
DECORATE(uimage2DRect,          true,   false)
DECORATE(image2DMS,             true,   false)
DECORATE(iimage2DMS,            true,   false)
DECORATE(uimage2DMS,            true,   false)
DECORATE(image2DMSArray,        true,   false)
DECORATE(iimage2DMSArray,       true,   false)
DECORATE(uimage2DMSArray,       true,   false)
DECORATE(image3D,               true,   false)
DECORATE(iimage3D,              true,   false)
DECORATE(uimage3D,              true,   false)
DECORATE(imageCube,             true,   false)
DECORATE(iimageCube,            true,   false)
DECORATE(uimageCube,            true,   false)
DECORATE(imageCubeArray,        true,   false)
DECORATE(iimageCubeArray,       true,   false)
DECORATE(uimageCubeArray,       true,   false)
DECORATE(imageBuffer,           true,   false)
DECORATE(iimageBuffer,          true,   false)
DECORATE(uimageBuffer,          true,   false)
DECORATE(struct,                false,  false)
// vulkan only
DECORATE(texture1D,             true,   false)
DECORATE(texture1DArray,        true,   false)
DECORATE(itexture1D,            true,   false)
DECORATE(itexture1DArray,       true,   false)
DECORATE(utexture1D,            true,   false)
DECORATE(utexture1DArray,       true,   false)
DECORATE(texture2D,             true,   false)
DECORATE(texture2DArray,        true,   false)
DECORATE(itexture2D,            true,   false)
DECORATE(itexture2DArray,       true,   false)
DECORATE(utexture2D,            true,   false)
DECORATE(utexture2DArray,       true,   false)
DECORATE(texture2DRect,         true,   false)
DECORATE(itexture2DRect,        true,   false)
DECORATE(utexture2DRect,        true,   false)
DECORATE(texture2DMS,           true,   false)
DECORATE(itexture2DMS,          true,   false)
DECORATE(utexture2DMS,          true,   false)
DECORATE(texture2DMSArray,      true,   false)
DECORATE(itexture2DMSArray,     true,   false)
DECORATE(utexture2DMSArray,     true,   false)
DECORATE(texture3D,             true,   false)
DECORATE(itexture3D,            true,   false)
DECORATE(utexture3D,            true,   false)
DECORATE(textureCube,           true,   false)
DECORATE(itextureCube,          true,   false)
DECORATE(utextureCube,          true,   false)
DECORATE(textureCubeArray,      true,   false)
DECORATE(itextureCubeArray,     true,   false)
DECORATE(utextureCubeArray,     true,   false)
DECORATE(textureBuffer,         true,   false)
DECORATE(itextureBuffer,        true,   false)
DECORATE(utextureBuffer,        true,   false)
DECORATE(sampler,               true,   false)
DECORATE(samplerShadow,         true,   false)
DECORATE(subpassInput,          true,   false)
DECORATE(isubpassInput,         true,   false)
DECORATE(usubpassInput,         true,   false)
DECORATE(subpassInputMS,        true,   false)
DECORATE(isubpassInputMS,       true,   false)
DECORATE(usubpassInputMS,       true,   false)
// reserved
DECORATE(common,                false,  true)
DECORATE(partition,             false,  true)
DECORATE(active,                false,  true)
DECORATE(asm,                   false,  true)
DECORATE(class,                 false,  true)
DECORATE(union,                 false,  true)
DECORATE(enum,                  false,  true)
DECORATE(typedef,               false,  true)
DECORATE(template,              false,  true)
DECORATE(this,                  false,  true)
DECORATE(resource,              false,  true)
DECORATE(goto,                  false,  true)
DECORATE(inline,                false,  true)
DECORATE(noinline,              false,  true)
DECORATE(public,                false,  true)
DECORATE(static,                false,  true)
DECORATE(extern,                false,  true)
DECORATE(external,              false,  true)
DECORATE(interface,             false,  true)
DECORATE(long,                  true,   true)
DECORATE(short,                 true,   true)
DECORATE(half,                  true,   true)
DECORATE(fixed,                 false,  true)
DECORATE(unsigned,              true,   true)
DECORATE(superp,                false,  true)
DECORATE(input,                 false,  true)
DECORATE(output,                false,  true)
DECORATE(hvec2,                 true,   true)
DECORATE(hvec3,                 true,   true)
DECORATE(hvec4,                 true,   true)
DECORATE(fvec2,                 true,   true)
DECORATE(fvec3,                 true,   true)
DECORATE(fvec4,                 true,   true)
DECORATE(filter,                false,  true)
DECORATE(sizeof,                false,  true)
DECORATE(cast,                  false,  true)
DECORATE(namespace,             false,  true)
DECORATE(using,                 false,  true)
DECORATE(sampler3DRect,         true,   true)
// raytracing stuff
DECORATE(accelerationStructureEXT, true, false)
DECORATE(rayPayloadEXT,         false,  false)
DECORATE(rayPayloadInEXT,       false,  false)
DECORATE(hitAttributeEXT,       false,  false)
DECORATE(callableDataEXT,       false,  false)
DECORATE(callableDataInEXT,     false,  false)
DECORATE(ignoreIntersectionEXT, false,  false)
DECORATE(terminateRayEXT,       false,  false)
DECORATE(rayQueryEXT,           true,   false)
// explicit types
DECORATE(float64_t,             true,   false)
DECORATE(f64vec2,               true,   false)
DECORATE(f64vec3,               true,   false)
DECORATE(f64vec4,               true,   false)
DECORATE(f64mat2,               true,   false)
DECORATE(f64mat3,               true,   false)
DECORATE(f64mat4,               true,   false)
DECORATE(f64mat2x2,             true,   false)
DECORATE(f64mat2x3,             true,   false)
DECORATE(f64mat2x4,             true,   false)
DECORATE(f64mat3x2,             true,   false)
DECORATE(f64mat3x3,             true,   false)
DECORATE(f64mat3x4,             true,   false)
DECORATE(f64mat4x2,             true,   false)
DECORATE(f64mat4x3,             true,   false)
DECORATE(f64mat4x4,             true,   false)
DECORATE(float32_t,             true,   false)
DECORATE(f32vec2,               true,   false)
DECORATE(f32vec3,               true,   false)
DECORATE(f32vec4,               true,   false)
DECORATE(f32mat2,               true,   false)
DECORATE(f32mat3,               true,   false)
DECORATE(f32mat4,               true,   false)
DECORATE(f32mat2x2,             true,   false)
DECORATE(f32mat2x3,             true,   false)
DECORATE(f32mat2x4,             true,   false)
DECORATE(f32mat3x2,             true,   false)
DECORATE(f32mat3x3,             true,   false)
DECORATE(f32mat3x4,             true,   false)
DECORATE(f32mat4x2,             true,   false)
DECORATE(f32mat4x3,             true,   false)
DECORATE(f32mat4x4,             true,   false)
DECORATE(float16_t,             true,   false)
DECORATE(f16vec2,               true,   false)
DECORATE(f16vec3,               true,   false)
DECORATE(f16vec4,               true,   false)
DECORATE(f16mat2,               true,   false)
DECORATE(f16mat3,               true,   false)
DECORATE(f16mat4,               true,   false)
DECORATE(f16mat2x2,             true,   false)
DECORATE(f16mat2x3,             true,   false)
DECORATE(f16mat2x4,             true,   false)
DECORATE(f16mat3x2,             true,   false)
DECORATE(f16mat3x3,             true,   false)
DECORATE(f16mat3x4,             true,   false)
DECORATE(f16mat4x2,             true,   false)
DECORATE(f16mat4x3,             true,   false)
DECORATE(f16mat4x4,             true,   false)
DECORATE(int64_t,               true,   false)
DECORATE(i64vec2,               true,   false)
DECORATE(i64vec3,               true,   false)
DECORATE(i64vec4,               true,   false)
DECORATE(uint64_t,              true,   false)
DECORATE(u64vec2,               true,   false)
DECORATE(u64vec3,               true,   false)
DECORATE(u64vec4,               true,   false)
DECORATE(int32_t,               true,   false)
DECORATE(i32vec2,               true,   false)
DECORATE(i32vec3,               true,   false)
DECORATE(i32vec4,               true,   false)
DECORATE(uint32_t,              true,   false)
DECORATE(u32vec2,               true,   false)
DECORATE(u32vec3,               true,   false)
DECORATE(u32vec4,               true,   false)
DECORATE(int16_t,               true,   false)
DECORATE(i16vec2,               true,   false)
DECORATE(i16vec3,               true,   false)
DECORATE(i16vec4,               true,   false)
DECORATE(uint16_t,              true,   false)
DECORATE(u16vec2,               true,   false)
DECORATE(u16vec3,               true,   false)
DECORATE(u16vec4,               true,   false)
DECORATE(int8_t,                true,   false)
DECORATE(i8vec2,                true,   false)
DECORATE(i8vec3,                true,   false)
DECORATE(i8vec4,                true,   false)
DECORATE(uint8_t,               true,   false)
DECORATE(u8vec2,                true,   false)
DECORATE(u8vec3,                true,   false)
DECORATE(u8vec4,                true,   false)
// generics
DECORATE(genFType,              true,   false)
DECORATE(genDType,              true,   false)
DECORATE(genIType,				true,	false)
DECORATE(genUType,				true,	false)
DECORATE(genBType,				true,	false)
DECORATE(vec,				    true,	false)
DECORATE(ivec,				    true,	false)
DECORATE(uvec,				    true,	false)
DECORATE(bvec,				    true,	false)
DECORATE(gvec4,					true,   false)
DECORATE(gsampler1D,		    true,	false)
DECORATE(gsampler1DArray,	    true,	false)
DECORATE(gsampler2D,		    true,	false)
DECORATE(gsampler2DRect,        true,	false)
DECORATE(gsampler2DArray,       true,   false)
DECORATE(gsampler2DMS,          true,   false)
DECORATE(gsampler2DMSArray,     true,   false)
DECORATE(gsampler3D,		    true,	false)
DECORATE(gsamplerCube,			true,	false)
DECORATE(gsamplerCubeArray,     true,   false)
DECORATE(gsamplerBuffer,        true,   false)
DECORATE(gimage1D,              true,   false)
DECORATE(gimage2D,              true,   false)
DECORATE(gimage3D,              true,   false)
DECORATE(gimageCube,            true,   false)
DECORATE(gimageCubeArray,       true,   false)
DECORATE(gimage2DArray,			true,	false)
DECORATE(gimage2DRect,			true,	false)
DECORATE(gimage1DArray,			true,	false)
DECORATE(gimage2DMS,			true,	false)
DECORATE(gimage2DMSArray,		true,	false)
DECORATE(gimageBuffer,			true,	false)
DECORATE(gsubpassInput,         true,   false)
DECORATE(gsubpassInputMS,       true,   false)