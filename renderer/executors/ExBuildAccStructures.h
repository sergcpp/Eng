#pragma once

#include "../Renderer_DrawList.h"
#include "../framegraph/FgNode.h"

namespace Phy {
struct prim_t;
struct split_settings_t;
} // namespace Phy

namespace Eng {
class ExBuildAccStructures final : public FgExecutor {
    const DrawList *&p_list_;
    int rt_index_;
    const AccelerationStructureData *acc_struct_data_;
    Ren::Span<const mesh_t> rt_meshes_;

    FgBufROHandle rt_obj_instances_buf_ro_;
    FgBufRWHandle rt_obj_instances_buf_rw_;
    FgBufRWHandle rt_tlas_buf_;
    FgBufRWHandle rt_tlas_build_scratch_buf_;

    void Execute_HWRT(const FgContext &fg);
    void Execute_SWRT(const FgContext &fg);

    static uint32_t PreprocessPrims_SAH(Ren::Span<const Phy::prim_t> prims, const Phy::split_settings_t &s,
                                        std::vector<gpu_bvh_node_t> &out_nodes, std::vector<uint32_t> &out_indices);
    static uint32_t ConvertToBVH2(Ren::Span<const gpu_bvh_node_t> nodes, std::vector<gpu_bvh2_node_t> &out_nodes);

  public:
    ExBuildAccStructures(const DrawList *&p_list, int rt_index, const FgBufROHandle rt_obj_instances_buf,
                         const AccelerationStructureData *acc_struct_data, Ren::Span<const mesh_t> rt_meshes,
                         const FgBufRWHandle rt_tlas_buf, const FgBufRWHandle rt_tlas_scratch_buf)
        : p_list_(p_list), rt_index_(rt_index), acc_struct_data_(acc_struct_data), rt_meshes_(rt_meshes),
          rt_obj_instances_buf_ro_(rt_obj_instances_buf), rt_tlas_buf_(rt_tlas_buf),
          rt_tlas_build_scratch_buf_(rt_tlas_scratch_buf) {}

    ExBuildAccStructures(const DrawList *&p_list, int rt_index, const FgBufRWHandle rt_obj_instances_buf,
                         const AccelerationStructureData *acc_struct_data, Ren::Span<const mesh_t> rt_meshes,
                         const FgBufRWHandle rt_tlas_buf, const FgBufRWHandle rt_tlas_scratch_buf)
        : p_list_(p_list), rt_index_(rt_index), acc_struct_data_(acc_struct_data), rt_meshes_(rt_meshes),
          rt_obj_instances_buf_rw_(rt_obj_instances_buf), rt_tlas_buf_(rt_tlas_buf),
          rt_tlas_build_scratch_buf_(rt_tlas_scratch_buf) {}

    void Execute(const FgContext &fg) override;
};
} // namespace Eng