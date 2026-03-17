#pragma once

#include <cfloat>
#include <memory>

#include "AccStructure.h"
#include "Anim.h"
#include "Buffer.h"
#include "Material.h"
#include "utils/Bitmask.h"
#include "utils/SmallVector.h"
#include "utils/Span.h"
#include "utils/String.h"

namespace Ren {
class ILog;

enum class eMeshFlags : uint8_t { HasAlpha = 0 };

struct tri_group_t {
    int byte_offset = -1, num_indices = 0;
    MaterialHandle front_mat, back_mat, vol_mat;
    Bitmask<eMeshFlags> flags;
};

struct vtx_delta_t {
    float dp[3], dn[3], db[3];
};

struct BufferRange {
    BufferHandle buf;
    SubAllocation sub;
};

enum class eMeshLoadStatus { Error, Found, CreatedFromData };

enum class eMeshType : uint8_t { Undefined, Simple, Colored, Skeletal };

using material_load_callback = std::function<std::array<MaterialHandle, 3>(std::string_view name)>;

enum class eMeshFileChunk { Info = 0, VtxAttributes, TriIndices, Materials, TriGroups, Bones, ShapeKeys };

struct mesh_chunk_pos_t {
    int32_t offset, length;
};
static_assert(sizeof(mesh_chunk_pos_t) == 8);

struct MeshFileInfo {
    char name[32] = "ModelName";
    float bbox_min[3] = {FLT_MAX, FLT_MAX, FLT_MAX}, bbox_max[3] = {-FLT_MAX, -FLT_MAX, -FLT_MAX};
};
static_assert(sizeof(MeshFileInfo) == 56);
static_assert(offsetof(MeshFileInfo, bbox_min) == 32);
static_assert(offsetof(MeshFileInfo, bbox_max) == 44);

struct MeshMain {
    eMeshType type = eMeshType::Undefined;
    Bitmask<eMeshFlags> flags;

    BufferRange attribs_buf1, attribs_buf2, indices_buf;
    BufferRange sk_attribs_buf, sk_deltas_buf;
};

struct MeshCold {
    String name;
    std::vector<float> attribs;
    std::vector<uint32_t> indices;
    std::vector<vtx_delta_t> deltas;
    SmallVector<tri_group_t, 8> groups;
    Vec3f bbox_min, bbox_max;
    Skeleton skel;
    AccStructHandle blas;
};

class Mesh : public RefCounter {
    eMeshType type_ = eMeshType::Undefined;
    Bitmask<eMeshFlags> flags_;
    bool ready_ = false;
    BufferRange attribs_buf1_, attribs_buf2_, indices_buf_;
    BufferRange sk_attribs_buf_, sk_deltas_buf_;
    std::vector<float> attribs_;
    std::vector<uint32_t> indices_;
    std::vector<vtx_delta_t> deltas_;
    SmallVector<tri_group_t, 8> groups_;
    Vec3f bbox_min_, bbox_max_;
    String name_;

    Skeleton skel_;

    // simple static mesh with normals
    void InitMeshSimple(std::istream &data, const material_load_callback &on_mat_load, const ApiContext &api,
                        DualStorage<BufferMain, BufferCold> &buffers, ResizableBuffer &vertex_buf1,
                        ResizableBuffer &vertex_buf2, ResizableBuffer &index_buf, ILog *log);
    // simple mesh with 4 per-vertex colors
    void InitMeshColored(std::istream &data, const material_load_callback &on_mat_load, const ApiContext &api,
                         DualStorage<BufferMain, BufferCold> &buffers, ResizableBuffer &vertex_buf1,
                         ResizableBuffer &vertex_buf2, ResizableBuffer &index_buf, ILog *log);
    // mesh with 4 bone weights per vertex
    void InitMeshSkeletal(std::istream &data, const material_load_callback &on_mat_load, const ApiContext &api,
                          DualStorage<BufferMain, BufferCold> &buffers, ResizableBuffer &skin_vertex_buf,
                          ResizableBuffer &delta_buf, ResizableBuffer &index_buf, ILog *log);

  public:
    Mesh() = default;
    Mesh(std::string_view name, const float *positions, int vtx_count, const uint32_t *indices, int ndx_count,
         const ApiContext &api, DualStorage<BufferMain, BufferCold> &buffers, ResizableBuffer &vertex_buf1,
         ResizableBuffer &vertex_buf2, ResizableBuffer &index_buf, eMeshLoadStatus *load_status, ILog *log);
    Mesh(std::string_view name, std::istream *data, const material_load_callback &on_mat_load, const ApiContext &api,
         DualStorage<BufferMain, BufferCold> &buffers, ResizableBuffer &vertex_buf1, ResizableBuffer &vertex_buf2,
         ResizableBuffer &index_buf, ResizableBuffer &skin_vertex_buf, ResizableBuffer &delta_buf,
         eMeshLoadStatus *load_status, ILog *log);

    Mesh(const Mesh &rhs) = delete;
    Mesh(Mesh &&rhs) = default;

    Mesh &operator=(const Mesh &rhs) = delete;
    Mesh &operator=(Mesh &&rhs) = default;

    eMeshType type() const { return type_; }
    Bitmask<eMeshFlags> flags() const { return flags_; }
    bool ready() const { return ready_; }
    BufferHandle attribs_buf1_handle() const { return attribs_buf1_.buf; }
    BufferHandle attribs_buf2_handle() const { return attribs_buf2_.buf; }
    BufferHandle indices_buf_handle() const { return indices_buf_.buf; }
    Span<const float> attribs() const { return attribs_; }
    const BufferRange &attribs_buf1() const { return attribs_buf1_; }
    const BufferRange &attribs_buf2() const { return attribs_buf2_; }
    const BufferRange &sk_attribs_buf() const { return sk_attribs_buf_; }
    const BufferRange &sk_deltas_buf() const { return sk_deltas_buf_; }
    Span<const uint32_t> indices() const { return indices_; }
    const BufferRange &indices_buf() const { return indices_buf_; }
    Span<const tri_group_t> groups() const { return groups_; }
    SmallVectorImpl<tri_group_t> &groups() { return groups_; }
    const Vec3f &bbox_min() const { return bbox_min_; }
    const Vec3f &bbox_max() const { return bbox_max_; }
    const String &name() const { return name_; }

    const Skeleton *skel() const { return &skel_; }
    Skeleton *skel() { return &skel_; }

    void Init(const float *positions, int vtx_count, const uint32_t *indices, int ndx_count, const ApiContext &api,
              DualStorage<BufferMain, BufferCold> &buffers, ResizableBuffer &vertex_buf1, ResizableBuffer &vertex_buf2,
              ResizableBuffer &index_buf, eMeshLoadStatus *load_status, ILog *log);
    void Init(std::istream *data, const material_load_callback &on_mat_load, const ApiContext &api,
              DualStorage<BufferMain, BufferCold> &buffers, ResizableBuffer &vertex_buf1, ResizableBuffer &vertex_buf2,
              ResizableBuffer &index_buf, ResizableBuffer &skin_vertex_buf, ResizableBuffer &delta_buf,
              eMeshLoadStatus *load_status, ILog *log);

    void InitBufferData(const ApiContext &api, DualStorage<BufferMain, BufferCold> &buffers,
                        ResizableBuffer &vertex_buf1, ResizableBuffer &vertex_buf2, ResizableBuffer &index_buf,
                        ILog *log);
    void ReleaseBufferData();

    AccStructHandle blas;
};

using MeshRef = StrongRef<Mesh, NamedStorage<Mesh>>;
using MeshStorage = NamedStorage<Mesh>;
} // namespace Ren