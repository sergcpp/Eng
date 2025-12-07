#include "Renderer_DrawList.h"

void Eng::DrawList::Init(
    const Ren::BufferHandle _shared_data_stage, const Ren::BufferHandle _instance_indices_stage,
    const Ren::BufferHandle _skin_transforms_stage, const Ren::BufferHandle _shape_keys_stage,
    const Ren::BufferHandle _cells_stage, const Ren::BufferHandle _rt_cells_stage, const Ren::BufferHandle _items_stage,
    const Ren::BufferHandle _rt_items_stage, const Ren::BufferHandle _lights_stage,
    const Ren::BufferHandle _decals_stage, const Ren::BufferHandle _rt_geo_instances_stage,
    const Ren::BufferHandle _rt_sh_geo_instances_stage, const Ren::BufferHandle _rt_vol_geo_instances_stage,
    const Ren::BufferHandle _rt_obj_instances_stage, const Ren::BufferHandle _rt_sh_obj_instances_stage,
    const Ren::BufferHandle _rt_vol_obj_instances_stage, const Ren::BufferHandle _rt_tlas_nodes_stage,
    const Ren::BufferHandle _rt_sh_tlas_nodes_stage, const Ren::BufferHandle _rt_vol_tlas_nodes_stage) {
    instance_indices_stage = _instance_indices_stage;
    shadow_lists.realloc(MAX_SHADOWMAPS_TOTAL);
    shadow_regions.realloc(MAX_SHADOWMAPS_TOTAL);
    skin_transforms_stage = _skin_transforms_stage;
    shape_keys_data.realloc(MAX_SHAPE_KEYS_TOTAL);
    shape_keys_stage = _shape_keys_stage;
    lights_stage = _lights_stage;
    decals_stage = _decals_stage;

    cells.realloc(ITEM_CELLS_COUNT);
    cells.count = ITEM_CELLS_COUNT;
    cells_stage = _cells_stage;
    rt_cells.realloc(ITEM_CELLS_COUNT);
    rt_cells.count = ITEM_CELLS_COUNT;
    rt_cells_stage = _rt_cells_stage;

    items.realloc(MAX_ITEMS_TOTAL);
    items_stage = _items_stage;
    rt_items.realloc(MAX_ITEMS_TOTAL);
    rt_items_stage = _rt_items_stage;

    for (int i = 0; i < int(eTLASIndex::_Count); ++i) {
        rt_geo_instances[i].realloc(MAX_RT_GEO_INSTANCES);
        rt_obj_instances[i].realloc(MAX_RT_OBJ_INSTANCES_TOTAL);
    }
    rt_geo_instances_stage[int(eTLASIndex::Main)] = _rt_geo_instances_stage;
    rt_geo_instances_stage[int(eTLASIndex::Shadow)] = _rt_sh_geo_instances_stage;
    rt_geo_instances_stage[int(eTLASIndex::Volume)] = _rt_vol_geo_instances_stage;
    rt_obj_instances_stage[int(eTLASIndex::Main)] = _rt_obj_instances_stage;
    rt_obj_instances_stage[int(eTLASIndex::Shadow)] = _rt_sh_obj_instances_stage;
    rt_obj_instances_stage[int(eTLASIndex::Volume)] = _rt_vol_obj_instances_stage;
    swrt.rt_tlas_nodes_stage[int(eTLASIndex::Main)] = _rt_tlas_nodes_stage;
    swrt.rt_tlas_nodes_stage[int(eTLASIndex::Shadow)] = _rt_sh_tlas_nodes_stage;
    swrt.rt_tlas_nodes_stage[int(eTLASIndex::Volume)] = _rt_vol_tlas_nodes_stage;

    shared_data_stage = _shared_data_stage;

    cached_shadow_regions.realloc(MAX_SHADOWMAPS_TOTAL);
}

void Eng::DrawList::Clear() {
    instance_indices.clear();
    shadow_batches.clear();
    shadow_batch_indices.clear();
    shadow_lists.count = 0;
    shadow_regions.count = 0;
    basic_batches.clear();
    basic_batch_indices.clear();
    custom_batches.clear();
    custom_batch_indices.clear();
    skin_transforms.clear();
    skin_regions.clear();
    shape_keys_data.count = 0;
    lights.clear();
    decals.clear();
    probes.clear();
    ellipsoids.clear();

    frame_index = 0;
    items.count = 0;
    rt_items.count = 0;

    visible_textures.clear();
    desired_textures.clear();

    decals_atlas = nullptr;
    // probe_storage = nullptr;

    cached_shadow_regions.count = 0;
}