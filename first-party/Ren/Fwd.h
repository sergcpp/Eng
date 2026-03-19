#pragma once

#include "Log.h"
#include "utils/Bitmask.h"
#include "utils/SparseStorage.h"

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
struct AccStructMain;
struct AccStructCold;
#if defined(REN_VK_BACKEND)
using CommandBuffer = VkCommandBuffer;
#else
using CommandBuffer = void *;
#endif
struct AnimSeqMain;
struct AnimSeqCold;
struct ApiContext;
struct BufferMain;
struct BufferCold;
class Camera;
class Context;
class DescrPool;
class DescrMultiPoolAlloc;
struct FramebufferMain;
struct FramebufferCold;
class ILog;
struct MaterialMain;
struct MaterialCold;
struct MeshMain;
struct MeshCold;
struct PipelineMain;
struct PipelineCold;
struct ProgramMain;
struct ProgramCold;
struct RastState;
struct RenderPass;
class ResizableBuffer;
struct Sampler;
struct ShaderMain;
struct ShaderCold;
struct ImageMain;
struct ImageCold;
class ImageAtlas;
class Texture2DAtlas;
class ImageSplitter;
struct VertexInput;

using AccStructRWHandle = Handle<AccStructMain, RWTag>;
using AccStructROHandle = Handle<AccStructMain, ROTag>;
using AccStructHandle = AccStructRWHandle;
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
using VertexInputRWHandle = Handle<VertexInput, RWTag>;
using VertexInputROHandle = Handle<VertexInput, ROTag>;
using VertexInputHandle = VertexInputRWHandle;
using PipelineRWHandle = Handle<PipelineMain, RWTag>;
using PipelineROHandle = Handle<PipelineMain, ROTag>;
using PipelineHandle = PipelineRWHandle;
using RenderPassRWHandle = Handle<RenderPass, RWTag>;
using RenderPassROHandle = Handle<RenderPass, ROTag>;
using RenderPassHandle = RenderPassRWHandle;
using SamplerRWHandle = Handle<Sampler, RWTag>;
using SamplerROHandle = Handle<Sampler, ROTag>;
using SamplerHandle = SamplerRWHandle;
using MaterialRWHandle = Handle<MaterialMain, RWTag>;
using MaterialROHandle = Handle<MaterialMain, ROTag>;
using MaterialHandle = MaterialRWHandle;
using MeshRWHandle = Handle<MeshMain, RWTag>;
using MeshROHandle = Handle<MeshMain, ROTag>;
using MeshHandle = MeshRWHandle;
using AnimSeqRWHandle = Handle<AnimSeqMain, RWTag>;
using AnimSeqROHandle = Handle<AnimSeqMain, ROTag>;
using AnimSeqHandle = AnimSeqRWHandle;

struct StoragesRef {
    SparseStorage<VertexInput, 8> &vtx_inputs;
    SparseDualStorage<ShaderMain, ShaderCold, 8> &shaders;
    SparseDualStorage<ProgramMain, ProgramCold, 8> &programs;
    SparseDualStorage<PipelineMain, PipelineCold, 8> &pipelines;
    SparseStorage<RenderPass, 8> &render_passes;
    SparseDualStorage<BufferMain, BufferCold, 8> &buffers;
    SparseDualStorage<ImageMain, ImageCold, 8> &images;
    SparseStorage<Sampler, 8> &samplers;
    SparseDualStorage<FramebufferMain, FramebufferCold, 8> &framebuffers;
    SparseDualStorage<AccStructMain, AccStructCold, 8> &acc_structs;
    SparseDualStorage<MaterialMain, MaterialCold, 8> &materials;
    SparseDualStorage<MeshMain, MeshCold, 8> &meshes;
    SparseDualStorage<AnimSeqMain, AnimSeqCold, 8> &anims;
};

const char *Version();
} // namespace Ren
