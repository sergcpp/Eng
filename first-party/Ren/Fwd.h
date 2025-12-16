#pragma once

#include "Bitmask.h"
#include "Log.h"
#include "Storage.h"

#if defined(REN_VK_BACKEND)
typedef uint64_t VkDeviceAddress;
typedef uint32_t VkFlags;
typedef VkFlags VkMemoryPropertyFlags;
struct VkVertexInputBindingDescription;
struct VkVertexInputAttributeDescription;
struct VkMemoryRequirements;
typedef struct VkAccelerationStructureKHR_T *VkAccelerationStructureKHR;
typedef struct VkRenderPass_T *VkRenderPass;
typedef struct VkFence_T *VkFence;
typedef struct VkDescriptorPool_T *VkDescriptorPool;
typedef struct VkDescriptorSet_T *VkDescriptorSet;
typedef struct VkDescriptorSetLayout_T *VkDescriptorSetLayout;
typedef struct VkBuffer_T *VkBuffer;
typedef struct VkBufferView_T *VkBufferView;
typedef struct VkDeviceMemory_T *VkDeviceMemory;
typedef struct VkCommandBuffer_T *VkCommandBuffer;
typedef struct VkDescriptorSet_T *VkDescriptorSet;
typedef struct VkDescriptorSetLayout_T *VkDescriptorSetLayout;
typedef struct VkImage_T *VkImage;
typedef struct VkImageView_T *VkImageView;
typedef struct VkSampler_T *VkSampler;
#elif defined(REN_GL_BACKEND)
typedef struct VkTraceRaysIndirectCommandKHR {
    uint32_t width;
    uint32_t height;
    uint32_t depth;
} VkTraceRaysIndirectCommandKHR;
#endif

namespace Ren {
#if defined(REN_VK_BACKEND)
class AccStructureVK;
using CommandBuffer = VkCommandBuffer;
#else
using CommandBuffer = void *;
#endif
class AnimSequence;
struct BufferMain;
struct BufferCold;
class Camera;
class Context;
class DescrPool;
class DescrMultiPoolAlloc;
class IAccStructure;
class ILog;
class Material;
class Mesh;
struct PipelineMain;
struct PipelineCold;
class ProbeStorage;
struct ProgramMain;
struct ProgramCold;
struct RastState;
struct RenderPassMain;
struct RenderPassCold;
struct SamplerMain;
struct SamplerCold;
struct ShaderMain;
struct ShaderCold;
class Image;
class ImageAtlas;
class Texture2DAtlas;
class ImageSplitter;
struct VertexInputMain;
struct VertexInputCold;

using AnimSeqRef = StrongRef<AnimSequence, NamedStorage<AnimSequence>>;
using MaterialRef = StrongRef<Material, NamedStorage<Material>>;
using MeshRef = StrongRef<Mesh, NamedStorage<Mesh>>;
using ImgRef = StrongRef<Image, NamedStorage<Image>>;
using WeakImgRef = WeakRef<Image, NamedStorage<Image>>;

using BufferHandle = Handle<BufferMain>;
using ShaderHandle = Handle<ShaderMain>;
using ProgramHandle = Handle<ProgramMain>;
using VertexInputHandle = Handle<VertexInputMain>;
using PipelineHandle = Handle<PipelineMain>;
using RenderPassHandle = Handle<RenderPassMain>;
using SamplerHandle = Handle<SamplerMain>;

struct StoragesRef {
    SortedDualStorage<VertexInputMain, VertexInputCold> &vtx_inputs;
    NamedDualStorage<ShaderMain, ShaderCold> &shaders;
    SortedDualStorage<ProgramMain, ProgramCold> &programs;
    SortedDualStorage<PipelineMain, PipelineCold> &pipelines;
    SortedDualStorage<RenderPassMain, RenderPassCold> &render_passes;
    NamedDualStorage<BufferMain, BufferCold> &buffers;
    SortedDualStorage<SamplerMain, SamplerCold> &samplers;
};

const char *Version();
} // namespace Ren
