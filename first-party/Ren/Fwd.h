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
struct ApiContext;
struct BufferMain;
struct BufferCold;
class Camera;
class Context;
class DescrPool;
class DescrMultiPoolAlloc;
struct FramebufferMain;
struct FramebufferCold;
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
class ResizableBuffer;
struct SamplerMain;
struct SamplerCold;
struct ShaderMain;
struct ShaderCold;
class Image;
struct ImageMain;
struct ImageCold;
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

using BufferRWHandle = Handle<BufferMain, RWTag>;
using BufferROHandle = Handle<BufferMain, ROTag>;
using BufferHandle = BufferRWHandle;
using FramebufferRWHandle = Handle<FramebufferMain, RWTag>;
using FramebufferROHandle = Handle<FramebufferMain, ROTag>;
using FramebufferHandle = FramebufferRWHandle;
using ImageRWHandle = Handle<ImageMain, RWTag>;
using ImageROHandle = Handle<ImageMain, ROTag>;
using ImageHandle = ImageRWHandle;
using ShaderRWHandle = Handle<ShaderMain, RWTag>;
using ShaderROHandle = Handle<ShaderMain, ROTag>;
using ShaderHandle = ShaderRWHandle;
using ProgramRWHandle = Handle<ProgramMain, RWTag>;
using ProgramROHandle = Handle<ProgramMain, ROTag>;
using ProgramHandle = ProgramRWHandle;
using VertexInputRWHandle = Handle<VertexInputMain, RWTag>;
using VertexInputROHandle = Handle<VertexInputMain, ROTag>;
using VertexInputHandle = VertexInputRWHandle;
using PipelineRWHandle = Handle<PipelineMain, RWTag>;
using PipelineROHandle = Handle<PipelineMain, ROTag>;
using PipelineHandle = PipelineRWHandle;
using RenderPassRWHandle = Handle<RenderPassMain, RWTag>;
using RenderPassROHandle = Handle<RenderPassMain, ROTag>;
using RenderPassHandle = RenderPassRWHandle;
using SamplerRWHandle = Handle<SamplerMain, RWTag>;
using SamplerROHandle = Handle<SamplerMain, ROTag>;
using SamplerHandle = SamplerRWHandle;

struct StoragesRef {
    DualStorage<VertexInputMain, VertexInputCold> &vtx_inputs;
    DualStorage<ShaderMain, ShaderCold> &shaders;
    DualStorage<ProgramMain, ProgramCold> &programs;
    DualStorage<PipelineMain, PipelineCold> &pipelines;
    DualStorage<RenderPassMain, RenderPassCold> &render_passes;
    DualStorage<BufferMain, BufferCold> &buffers;
    DualStorage<ImageMain, ImageCold> &images;
    DualStorage<SamplerMain, SamplerCold> &samplers;
    DualStorage<FramebufferMain, FramebufferCold> &framebuffers;
};

const char *Version();
} // namespace Ren
