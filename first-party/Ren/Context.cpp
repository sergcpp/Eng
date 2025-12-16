#include "Context.h"

#include <algorithm>
#include <istream>

#if defined(REN_VK_BACKEND)
#include "VKCtx.h"
#elif defined(REN_GL_BACKEND)
#include "GLCtx.h"
#endif

const char *Ren::Version() { return "v0.1.0-unknown-commit"; }

Ren::MeshRef Ren::Context::LoadMesh(std::string_view name, const float *positions, const int vtx_count,
                                    const uint32_t *indices, const int ndx_count, eMeshLoadStatus *load_status) {
    return LoadMesh(name, positions, vtx_count, indices, ndx_count, default_vertex_buf1_, default_vertex_buf2_,
                    default_indices_buf_, load_status);
}

Ren::MeshRef Ren::Context::LoadMesh(std::string_view name, const float *positions, const int vtx_count,
                                    const uint32_t *indices, const int ndx_count, const BufferHandle vertex_buf1,
                                    const BufferHandle vertex_buf2, const BufferHandle index_buf,
                                    eMeshLoadStatus *load_status) {
    MeshRef ref = meshes_.FindByName(name);
    if (!ref) {
        ref = meshes_.Insert(name, positions, vtx_count, indices, ndx_count, *api_, buffers_, vertex_buf1, vertex_buf2,
                             index_buf, load_status, log_);
    } else {
        if (ref->ready()) {
            (*load_status) = eMeshLoadStatus::Found;
        } else if (positions) {
            ref->Init(positions, vtx_count, indices, ndx_count, *api_, buffers_, vertex_buf1, vertex_buf2, index_buf,
                      load_status, log_);
        }
    }

    return ref;
}

Ren::MeshRef Ren::Context::LoadMesh(std::string_view name, std::istream *data,
                                    const material_load_callback &on_mat_load, eMeshLoadStatus *load_status) {
    return LoadMesh(name, data, on_mat_load, default_vertex_buf1_, default_vertex_buf2_, default_indices_buf_,
                    default_skin_vertex_buf_, default_delta_vertex_buf_, load_status);
}

Ren::MeshRef Ren::Context::LoadMesh(std::string_view name, std::istream *data,
                                    const material_load_callback &on_mat_load, const BufferHandle vertex_buf1,
                                    const BufferHandle vertex_buf2, const BufferHandle index_buf,
                                    const BufferHandle skin_vertex_buf, const BufferHandle delta_buf,
                                    eMeshLoadStatus *load_status) {
    MeshRef ref = meshes_.FindByName(name);
    if (!ref) {
        ref = meshes_.Insert(name, data, on_mat_load, *api_, buffers_, vertex_buf1, vertex_buf2, index_buf,
                             skin_vertex_buf, delta_buf, load_status, log_);
    } else {
        if (ref->ready()) {
            (*load_status) = eMeshLoadStatus::Found;
        } else if (data) {
            ref->Init(data, on_mat_load, *api_, buffers_, vertex_buf1, vertex_buf2, index_buf, skin_vertex_buf,
                      delta_buf, load_status, log_);
        }
    }

    return ref;
}

Ren::MaterialRef Ren::Context::LoadMaterial(std::string_view name, std::string_view mat_src, eMatLoadStatus *status,
                                            const pipelines_load_callback &on_pipes_load,
                                            const texture_load_callback &on_tex_load,
                                            const sampler_load_callback &on_sampler_load) {
    MaterialRef ref = materials_.FindByName(name);
    if (!ref) {
        ref = materials_.Insert(name, mat_src, status, on_pipes_load, on_tex_load, on_sampler_load, log_);
    } else {
        if (ref->ready()) {
            (*status) = eMatLoadStatus::Found;
        } else if (!ref->ready() && !mat_src.empty()) {
            ref->Init(mat_src, status, on_pipes_load, on_tex_load, on_sampler_load, log_);
        }
    }

    return ref;
}

int Ren::Context::NumMaterialsNotReady() {
    return int(std::count_if(materials_.begin(), materials_.end(), [](const Material &m) { return !m.ready(); }));
}

void Ren::Context::ReleaseMaterials() {
    if (materials_.empty()) {
        return;
    }
    log_->Error("---------REMAINING MATERIALS--------");
    for (const Material &m : materials_) {
        log_->Error("%s", m.name().c_str());
    }
    log_->Error("-----------------------------------");
    materials_.clear();
}

#if defined(REN_GL_BACKEND)
Ren::ShaderHandle Ren::Context::LoadShader(std::string_view name, std::string_view shader_src, const eShaderType type) {
    ShaderHandle ret = shaders_.Find(name);
    if (!ret) {
        const Ren::String name_str{name};
        ret = shaders_.Emplace(name_str);

        const auto &[shader_main, shader_cold] = shaders_.Get(ret);
        if (!Shader_Init(*api_, shader_main, shader_cold, shader_src, name_str, type, log_)) {
            shaders_.Release(name);
            return {};
        }
    }
    return ret;
}
#endif

Ren::ShaderHandle Ren::Context::LoadShader(std::string_view name, Span<const uint8_t> spirv_data,
                                           const eShaderType type) {
    ShaderHandle ret = shaders_.Find(name);
    if (!ret) {
        const Ren::String name_str{name};
        ret = shaders_.Emplace(name_str);

        const auto &[shader_main, shader_cold] = shaders_.Get(ret);
        if (!Shader_Init(*api_, shader_main, shader_cold, spirv_data, name_str, type, log_)) {
            shaders_.Release(name);
            return {};
        }
    }
    return ret;
}

Ren::ProgramHandle Ren::Context::LoadProgram(const ShaderHandle vs, const ShaderHandle fs, const ShaderHandle tcs,
                                             const ShaderHandle tes, const ShaderHandle gs) {
    std::array<ShaderHandle, int(eShaderType::_Count)> temp_shaders;
    temp_shaders[int(eShaderType::Vertex)] = vs;
    temp_shaders[int(eShaderType::Fragment)] = fs;
    temp_shaders[int(eShaderType::TesselationControl)] = tcs;
    temp_shaders[int(eShaderType::TesselationEvaluation)] = tes;
    temp_shaders[int(eShaderType::Geometry)] = gs;
    ProgramHandle ret = programs_.LowerBound([&](const ProgramMain &p) { return p.shaders < temp_shaders; });
    if (!ret || programs_.Get(ret).first.shaders != temp_shaders) {
        assert(vs && fs);

        ProgramMain prog_main;
        ProgramCold prog_cold;

        if (!Program_Init(*api_, shaders_, prog_main, prog_cold, vs, fs, tcs, tes, gs, log_)) {
            return {};
        }

        ret = programs_.Insert(std::move(prog_main), std::move(prog_cold));
        assert(programs_.CheckUnique());
    }
    return ret;
}

Ren::ProgramHandle Ren::Context::LoadProgram(const ShaderHandle cs) {
    std::array<Ren::ShaderHandle, int(Ren::eShaderType::_Count)> temp_shaders;
    temp_shaders[int(Ren::eShaderType::Compute)] = cs;
    ProgramHandle ret = programs_.LowerBound([&](const ProgramMain &p) { return p.shaders < temp_shaders; });
    if (!ret || programs_.Get(ret).first.shaders != temp_shaders) {
        assert(cs);

        ProgramMain prog_main;
        ProgramCold prog_cold;

        if (!Program_Init(*api_, shaders_, prog_main, prog_cold, cs, log_)) {
            return {};
        }

        ret = programs_.Insert(std::move(prog_main), std::move(prog_cold));
        assert(programs_.CheckUnique());
    }
    return ret;
}

#if defined(REN_VK_BACKEND)
Ren::ProgramHandle Ren::Context::LoadProgram2(const ShaderHandle raygen, const ShaderHandle closesthit,
                                              const ShaderHandle anyhit, const ShaderHandle miss,
                                              const ShaderHandle intersection) {
    std::array<Ren::ShaderHandle, int(Ren::eShaderType::_Count)> temp_shaders;
    temp_shaders[int(Ren::eShaderType::RayGen)] = raygen;
    temp_shaders[int(Ren::eShaderType::ClosestHit)] = closesthit;
    temp_shaders[int(Ren::eShaderType::AnyHit)] = anyhit;
    temp_shaders[int(Ren::eShaderType::Miss)] = miss;
    temp_shaders[int(Ren::eShaderType::Intersection)] = intersection;
    ProgramHandle ret = programs_.LowerBound([&](const ProgramMain &p) { return p.shaders < temp_shaders; });
    if (!ret || programs_.Get(ret).first.shaders != temp_shaders) {
        assert(raygen);

        ProgramMain prog_main;
        ProgramCold prog_cold;

        if (!Program_Init(*api_, shaders_, prog_main, prog_cold, raygen, closesthit, anyhit, miss, intersection,
                          log_)) {
            return {};
        }

        ret = programs_.Insert(std::move(prog_main), std::move(prog_cold));
        assert(programs_.CheckUnique());
    }
    return ret;
}
#endif

void Ren::Context::ReleasePrograms() {
    if (programs_.Empty()) {
        return;
    }
    log_->Error("---------REMAINING PROGRAMS--------");
    while (!programs_.sorted_items().empty()) {
        const ProgramHandle p = programs_.sorted_items().back();
        const auto &[p_main, p_cold] = programs_.Get(p);
        Program_Destroy(*api_, p_main, p_cold);
        programs_.PopBack();
    }
    log_->Error("-----------------------------------");
}

Ren::VertexInputHandle Ren::Context::FindOrCreateVertexInput(Span<const VtxAttribDesc> attribs,
                                                             const BufferHandle elem_buf) {
    VertexInputHandle ret = vtx_inputs_.LowerBound([&](const VertexInputMain &vi) {
        if (vi.elem_buf < elem_buf) {
            return true;
        } else if (vi.elem_buf == elem_buf) {
            return Span<const VtxAttribDesc>(vi.attribs) < attribs;
        }
        return false;
    });
    if (!ret || vtx_inputs_.Get(ret).first.elem_buf != elem_buf ||
        Span<const VtxAttribDesc>(vtx_inputs_.Get(ret).first.attribs) != attribs) {
        VertexInputMain main = {};

        if (!VertexInput_Init(main, attribs, elem_buf)) {
            return {};
        }

        ret = vtx_inputs_.Insert(std::move(main), {});
    }
    return ret;
}

void Ren::Context::ReleaseVertexInput(const VertexInputHandle handle) { vtx_inputs_.Free(handle); }

void Ren::Context::ReleaseVertexInputs() {
    if (vtx_inputs_.Empty()) {
        return;
    }
    // log_->Error("--------REMAINING VTX INPUTS-------");
    while (!vtx_inputs_.sorted_items().empty()) {
        const VertexInputHandle vi = vtx_inputs_.sorted_items().back();
        const auto &[vi_main, vi_cold] = vtx_inputs_.Get(vi);
        // if (vi_main.elem_buf) {
        //     log_->Error("%i attribs + elem buf", int(vi_main.attribs.size()));
        // } else {
        //     log_->Error("%i attribs", int(vi_main.attribs.size()));
        // }
        VertexInput_Destroy(vi_main);
        vtx_inputs_.PopBack();
    }
    // log_->Error("-----------------------------------");
}

Ren::RenderPassHandle Ren::Context::FindOrCreateRenderPass(const RenderTargetInfo &depth_rt,
                                                           Span<const RenderTargetInfo> color_rts) {
    RenderPassHandle ret =
        render_passes_.LowerBound([&](const Ren::RenderPassMain &rp) { return rp.LessThan(depth_rt, color_rts); });
    if (!ret || !render_passes_.Get(ret).first.Equals(depth_rt, color_rts)) {
        Ren::RenderPassMain rp_main = {};
        if (!RenderPass_Init(*api_, rp_main, depth_rt, color_rts, log_)) {
            return {};
        }
        ret = render_passes_.Insert(std::move(rp_main), {});
        assert(render_passes_.CheckUnique());
    }
    return ret;
}

void Ren::Context::ReleaseRenderPass(const RenderPassHandle handle) { render_passes_.Free(handle); }

void Ren::Context::ReleaseRenderPasses() {
    if (render_passes_.Empty()) {
        return;
    }
    // log_->Error("------REMAINING RENDER PASSES------");
    while (!render_passes_.sorted_items().empty()) {
        const RenderPassHandle vi = render_passes_.sorted_items().back();
        const auto &[rp_main, rp_cold] = render_passes_.Get(vi);
        RenderPass_Destroy(*api_, rp_main);
        render_passes_.PopBack();
    }
    // log_->Error("-----------------------------------");
}

Ren::PipelineHandle Ren::Context::FindOrCreatePipeline(const ProgramHandle prog, const int subgroup_size) {
    PipelineHandle ret = pipelines_.LowerBound([&](const PipelineMain &pi) { return pi.LessThan({}, prog, {}, {}); });
    if (!ret || !pipelines_.Get(ret).first.Equals({}, prog, {}, {})) {
        assert(prog);

        Ren::PipelineMain pipeline_main;
        Ren::PipelineCold pipeline_cold;
        if (!Pipeline_Init(*api_, shaders_, programs_, buffers_, pipeline_main, pipeline_cold, prog, log_,
                           subgroup_size)) {
            return {};
        }

        ret = pipelines_.Insert(std::move(pipeline_main), std::move(pipeline_cold));
        assert(pipelines_.CheckUnique());
    }
    return ret;
}

Ren::PipelineHandle Ren::Context::FindOrCreatePipeline(const RastState &rast_state, const ProgramHandle prog,
                                                       const VertexInputHandle vtx_input,
                                                       const RenderPassHandle render_pass,
                                                       const uint32_t subpass_index) {
    PipelineHandle ret = pipelines_.LowerBound(
        [&](const PipelineMain &pi) { return pi.LessThan(rast_state, prog, vtx_input, render_pass); });
    if (!ret || !pipelines_.Get(ret).first.Equals(rast_state, prog, vtx_input, render_pass)) {
        assert(prog);

        PipelineMain pipeline_main;
        PipelineCold pipeline_cold;
        if (!Pipeline_Init(*api_, storages_, pipeline_main, pipeline_cold, rast_state, prog, vtx_input, render_pass,
                           subpass_index, log_)) {
            return {};
        }

        ret = pipelines_.Insert(std::move(pipeline_main), std::move(pipeline_cold));
        assert(pipelines_.CheckUnique());
    }
    return ret;
}

Ren::ImgRef Ren::Context::LoadImage(std::string_view name, const ImgParams &p, MemAllocators *mem_allocs,
                                    eImgLoadStatus *load_status) {
    ImgRef ref = images_.FindByName(name);
    if (!ref) {
        ref = images_.Insert(name, api_.get(), p, mem_allocs, log_);
        (*load_status) = eImgLoadStatus::CreatedDefault;
    } else if (ref->params != p) {
        ref->Init(p, mem_allocs, log_);
        (*load_status) = eImgLoadStatus::Reinitialized;
    } else {
        (*load_status) = eImgLoadStatus::Found;
    }
    return ref;
}

Ren::ImgRef Ren::Context::LoadImage(std::string_view name, const ImgHandle &handle, const ImgParams &p,
                                    MemAllocation &&alloc, eImgLoadStatus *load_status) {
    ImgRef ref = images_.FindByName(name);
    if (!ref) {
        ref = images_.Insert(name, api_.get(), handle, p, std::move(alloc), log_);
        (*load_status) = eImgLoadStatus::CreatedDefault;
    } else if (ref->params != p) {
        ref->Init(handle, p, std::move(alloc), log_);
        (*load_status) = eImgLoadStatus::Reinitialized;
    } else {
        (*load_status) = eImgLoadStatus::Found;
    }
    return ref;
}

Ren::ImgRef Ren::Context::LoadImage(std::string_view name, Span<const uint8_t> data, const ImgParams &p,
                                    MemAllocators *mem_allocs, eImgLoadStatus *load_status) {
    ImgRef ref = images_.FindByName(name);
    if (!ref) {
        ref = images_.Insert(name, api_.get(), data, p, mem_allocs, load_status, log_);
    } else {
        (*load_status) = eImgLoadStatus::Found;
        if ((Bitmask<eImgFlags>{ref->params.flags} & eImgFlags::Stub) && !(p.flags & eImgFlags::Stub) &&
            !data.empty()) {
            ref->Init(data, p, mem_allocs, load_status, log_);
        }
    }
    return ref;
}

Ren::ImgRef Ren::Context::LoadImageCube(std::string_view name, Span<const uint8_t> data[6], const ImgParams &p,
                                        MemAllocators *mem_allocs, eImgLoadStatus *load_status) {
    ImgRef ref = images_.FindByName(name);
    if (!ref) {
        ref = images_.Insert(name, api_.get(), data, p, mem_allocs, load_status, log_);
    } else {
        (*load_status) = eImgLoadStatus::Found;
        if ((Bitmask<eImgFlags>{ref->params.flags} & eImgFlags::Stub) && (p.flags & eImgFlags::Stub) && data) {
            ref->Init(data, p, mem_allocs, load_status, log_);
        }
    }

    return ref;
}

void Ren::Context::VisitImages(eImgFlags mask, const std::function<void(Image &tex)> &callback) {
    for (Image &tex : images_) {
        if (Bitmask<eImgFlags>{tex.params.flags} & mask) {
            callback(tex);
        }
    }
}

int Ren::Context::NumImagesNotReady() {
    return int(std::count_if(images_.begin(), images_.end(),
                             [](const Image &t) { return Bitmask<eImgFlags>{t.params.flags} & eImgFlags::Stub; }));
}

void Ren::Context::ReleaseImages() {
    if (images_.empty()) {
        return;
    }
    log_->Error("----------REMAINING IMAGES---------");
    for (const Image &t : images_) {
        log_->Error("%s", t.name().c_str());
    }
    log_->Error("-----------------------------------");
    images_.clear();
}

Ren::ImageRegionRef Ren::Context::LoadImageRegion(std::string_view name, Span<const uint8_t> data, const ImgParams &p,
                                                  CommandBuffer cmd_buf, eImgLoadStatus *load_status) {
    ImageRegionRef ref = image_regions_.FindByName(name);
    if (!ref) {
        ref = image_regions_.Insert(name, data, p, cmd_buf, &image_atlas_, load_status, log_);
    } else {
        if (ref->ready()) {
            (*load_status) = eImgLoadStatus::Found;
        } else {
            ref->Init(data, p, cmd_buf, &image_atlas_, load_status, log_);
        }
    }
    return ref;
}

Ren::ImageRegionRef Ren::Context::LoadImageRegion(std::string_view name, const BufferMain &sbuf, const int data_off,
                                                  const int data_len, const ImgParams &p, CommandBuffer cmd_buf,
                                                  eImgLoadStatus *load_status) {
    ImageRegionRef ref = image_regions_.FindByName(name);
    if (!ref) {
        ref = image_regions_.Insert(name, sbuf, data_off, data_len, p, cmd_buf, &image_atlas_, load_status, log_);
    } else {
        if (ref->ready()) {
            (*load_status) = eImgLoadStatus::Found;
        } else {
            ref->Init(sbuf, data_off, data_len, p, cmd_buf, &image_atlas_, load_status, log_);
        }
    }
    return ref;
}

void Ren::Context::ReleaseTextureRegions() {
    if (image_regions_.empty()) {
        return;
    }
    log_->Error("-------REMAINING TEX REGIONS-------");
    for (const ImageRegion &t : image_regions_) {
        log_->Error("%s", t.name().c_str());
    }
    log_->Error("-----------------------------------");
    image_regions_.clear();
}

Ren::SamplerHandle Ren::Context::FindOrCreateSampler(const SamplingParams params) {
    SamplerHandle ret = samplers_.LowerBound([&](const SamplerMain &s) { return SamplingParams(s.params) < params; });
    if (!ret || samplers_.Get(ret).first.params != params) {
        SamplerMain s_main = {};
        SamplerCold s_cold = {};

        if (!Sampler_Init(*api_, s_main, s_cold, params)) {
            return {};
        }

        ret = samplers_.Insert(std::move(s_main), std::move(s_cold));
        assert(samplers_.CheckUnique());
    }
    return ret;
}

void Ren::Context::ReleaseSamplers() {
    if (samplers_.Empty()) {
        return;
    }
    // log_->Error("--------REMAINING SAMPLERS---------");
    while (!samplers_.sorted_items().empty()) {
        const SamplerHandle s = samplers_.sorted_items().back();
        const auto &[s_main, s_cold] = samplers_.Get(s);
        Sampler_Destroy(*api_, s_main, s_cold);
        samplers_.PopBack();
    }
    // log_->Error("-----------------------------------");
}

Ren::AnimSeqRef Ren::Context::LoadAnimSequence(std::string_view name, std::istream &data) {
    AnimSeqRef ref = anims_.FindByName(name);
    if (!ref) {
        ref = anims_.Insert(name, data);
    } else {
        if (ref->ready()) {
        } else if (!ref->ready() && data) {
            ref->Init(data);
        }
    }
    return ref;
}

int Ren::Context::NumAnimsNotReady() {
    return int(std::count_if(anims_.begin(), anims_.end(), [](const AnimSequence &a) { return !a.ready(); }));
}

void Ren::Context::ReleaseAnims() {
    if (anims_.empty()) {
        return;
    }
    log_->Error("---------REMAINING ANIMS--------");
    for (const AnimSequence &a : anims_) {
        log_->Error("%s", a.name().c_str());
    }
    log_->Error("-----------------------------------");
    anims_.clear();
}

Ren::BufferHandle Ren::Context::FindOrCreateBuffer(std::string_view name, const eBufType type,
                                                   const uint32_t initial_size, const uint32_t size_alignment,
                                                   MemAllocators *mem_allocs) {
    BufferHandle ret = buffers_.Find(name);
    if (!ret) {
        String name_str{name};
        ret = buffers_.Emplace(name_str);

        const auto &[buf_main, buf_cold] = buffers_.Get(ret);

        if (!Buffer_Init(*api_, buf_main, buf_cold, name_str, type, initial_size, log_, size_alignment, mem_allocs)) {
            buffers_.Release(name_str);
            ret = {};
        }
    }
    return ret;
}

Ren::BufferHandle Ren::Context::CreateBuffer(std::string_view name, const eBufType type, const BufferMain &_buf_main,
                                             MemAllocation &&alloc, const uint32_t initial_size,
                                             const uint32_t size_alignment) {
    BufferHandle ret = buffers_.Find(name);
    assert(!ret);
    { // Add new buffer
        String name_str{name};
        ret = buffers_.Emplace(name_str);

        const auto &[buf_main, buf_cold] = buffers_.Get(ret);

        buf_main = _buf_main;
        if (!Buffer_Init(*api_, buf_cold, name_str, type, std::move(alloc), initial_size, log_, size_alignment)) {
            buffers_.Release(name_str);
            ret = {};
        }
    }
    return ret;
}

int Ren::Context::FindOrCreateBufferView(const BufferHandle handle, const eFormat format) {
    const auto &[buf_main, buf_cold] = buffers_.Get(handle);
    for (int i = 0; i < int(buf_main.views.size()); ++i) {
        if (buf_main.views[i].first == format) {
            return i;
        }
    }
    return Buffer_AddView(*api_, buf_main, buf_cold, format);
}

bool Ren::Context::ResizeBuffer(const BufferHandle handle, const uint32_t new_size, const bool keep_content,
                                const bool release_immediately) {
    const auto &[buf_main, buf_cold] = buffers_.Get(handle);
    return Buffer_Resize(*api_, buf_main, buf_cold, new_size, log_, keep_content, release_immediately);
}

uint8_t *Ren::Context::MapBufferRange(const BufferHandle handle, const uint32_t offset, const uint32_t size,
                                      const bool persistent) {
    const auto &[buf_main, buf_cold] = buffers_.Get(handle);
    return Buffer_MapRange(*api_, buf_main, buf_cold, offset, size, persistent);
}

uint8_t *Ren::Context::MapBuffer(const BufferHandle handle, const bool persistent) {
    const auto &[buf_main, buf_cold] = buffers_.Get(handle);
    return Buffer_MapRange(*api_, buf_main, buf_cold, 0, buf_cold.size, persistent);
}

void Ren::Context::UnmapBuffer(const BufferHandle handle) {
    const auto &[buf_main, buf_cold] = buffers_.Get(handle);
    Buffer_Unmap(*api_, buf_main, buf_cold);
}

void Ren::Context::ReleaseBuffer(const BufferHandle handle, const bool immediately) {
    const auto &[buf_main, buf_cold] = buffers_.Get(handle);
    const String name_str = buf_cold.name;
    if (immediately) {
        Buffer_DestroyImmediately(*api_, buf_main, buf_cold);
    } else {
        Buffer_Destroy(*api_, buf_main, buf_cold);
    }
    buffers_.Release(name_str);
}

void Ren::Context::ReleaseBuffers() {
    if (buffers_.Empty()) {
        return;
    }
    log_->Error("---------REMAINING BUFFERS--------");
    auto &all_buffers = buffers_.items_by_name();
    for (auto it = all_buffers.begin(); it != all_buffers.end();) {
        const auto &[buf_main, buf_cold] = buffers_.Get(it->val);
        const String name_str = buf_cold.name;
        log_->Error("%s\t: %u", name_str.c_str(), buf_cold.size);

        Buffer_Destroy(*api_, buf_main, buf_cold);
        it = buffers_.Free(it);
    }
    log_->Error("-----------------------------------");
}

void Ren::Context::InitDefaultBuffers() {
    assert(!default_vertex_buf1_);
    assert(!default_vertex_buf2_);
    assert(!default_indices_buf_);
    assert(!default_skin_vertex_buf_);
    assert(!default_delta_vertex_buf_);

    { // Vertex Buffer 1
        default_vertex_buf1_ = FindOrCreateBuffer("Default Vtx Buf 1", eBufType::VertexAttribs, 1 * 1024 * 1024, 16);
        const int view_index = FindOrCreateBufferView(default_vertex_buf1_, eFormat::RGBA32F);
        assert(view_index == 0);
    }
    { // Vertex Buffer 2
        default_vertex_buf2_ = FindOrCreateBuffer("Default Vtx Buf 2", eBufType::VertexAttribs, 1 * 1024 * 1024, 16);
        const int view_index = FindOrCreateBufferView(default_vertex_buf2_, eFormat::RGBA32UI);
        assert(view_index == 0);
    }
    { // Index Buffer
        default_indices_buf_ = FindOrCreateBuffer("Default Ndx Buf", eBufType::VertexIndices, 1 * 1024 * 1024, 4);
        const int view_index = FindOrCreateBufferView(default_indices_buf_, eFormat::R32UI);
        assert(view_index == 0);
    }
    { // Skin Buffer
        default_skin_vertex_buf_ =
            FindOrCreateBuffer("Default Skin Vtx Buf", eBufType::VertexAttribs, 1 * 1024 * 1024, 16);
    }
    { // Delta Shapes Buffer
        default_delta_vertex_buf_ =
            FindOrCreateBuffer("Default Delta Vtx Buf", eBufType::VertexAttribs, 1 * 1024 * 1024, 16);
    }
}

void Ren::Context::ReleaseDefaultBuffers() {
    ReleaseBuffer(default_vertex_buf1_);
    default_vertex_buf1_ = {};
    ReleaseBuffer(default_vertex_buf2_);
    default_vertex_buf2_ = {};
    ReleaseBuffer(default_indices_buf_);
    default_indices_buf_ = {};
    ReleaseBuffer(default_skin_vertex_buf_);
    default_skin_vertex_buf_ = {};
    ReleaseBuffer(default_delta_vertex_buf_);
    default_delta_vertex_buf_ = {};
}

void Ren::Context::ReleaseAll() {
    meshes_.clear();

    ReleaseDefaultBuffers();

    ReleaseAnims();
    ReleaseMaterials();
    ReleaseImages();
    ReleaseTextureRegions();
    ReleaseBuffers();
    ReleaseVertexInputs();
    ReleaseRenderPasses();
    ReleaseSamplers();

    image_atlas_ = {};
}

Ren::DescrMultiPoolAlloc &Ren::Context::default_descr_alloc() const {
    return *default_descr_alloc_[api_->backend_frame];
}

int Ren::Context::backend_frame() const { return api_->backend_frame; }

int Ren::Context::active_present_image() const { return api_->active_present_image; }

Ren::ImgRef Ren::Context::backbuffer_ref() const { return api_->present_image_refs[api_->active_present_image]; }

Ren::StageBufRef::StageBufRef(Context &_ctx, const BufferHandle _buf, SyncFence &_fence, CommandBuffer _cmd_buf,
                              bool &_is_in_use)
    : ctx(_ctx), buf(_buf), fence(_fence), cmd_buf(_cmd_buf), is_in_use(_is_in_use) {
    is_in_use = true;
    const eWaitResult res = fence.ClientWaitSync();
    assert(res == eWaitResult::Success);
    ctx.BegSingleTimeCommands(cmd_buf);
}

Ren::StageBufRef::~StageBufRef() {
    if (buf) {
        fence = ctx.EndSingleTimeCommands(cmd_buf);
        is_in_use = false;
    }
}