#include "Renderer.h"

#include <Ren/Context.h>

#include "Renderer_Names.h"

#include "shaders/bloom_interface.h"
#include "shaders/histogram_exposure_interface.h"
#include "shaders/histogram_sample_interface.h"
#include "shaders/sharpen_interface.h"

Eng::FgImgRWHandle Eng::Renderer::AddAutoexposurePasses(const FgImgROHandle hdr_texture,
                                                        const Ren::Vec2f adaptation_speed) {
    FgImgRWHandle histogram;
    { // Clear histogram image
        auto &histogram_clear = fg_builder_.AddNode("HISTOGRAM CLEAR");

        FgImgDesc desc;
        desc.w = EXPOSURE_HISTOGRAM_RES + 1;
        desc.h = 1;
        desc.format = Ren::eFormat::R32UI;
        desc.sampling.wrap = Ren::eWrap::ClampToEdge;

        histogram = histogram_clear.AddClearImageOutput("Exposure Histogram", desc);

        histogram_clear.set_execute_cb([histogram](const FgContext &fg) {
            const Ren::ImageRWHandle histogram_tex = fg.AccessRWImage(histogram);

            fg.ren_ctx().CmdClearImage(histogram_tex, {}, fg.cmd_buf());
        });
    }
    { // Sample histogram
        auto &histogram_sample = fg_builder_.AddNode("HISTOGRAM SAMPLE");

        FgImgROHandle input = histogram_sample.AddTextureInput(hdr_texture, Ren::eStage::ComputeShader);
        histogram = histogram_sample.AddStorageImageOutput(histogram, Ren::eStage::ComputeShader);

        histogram_sample.set_execute_cb([this, input, histogram](const FgContext &fg) {
            const Ren::ImageROHandle input_tex = fg.AccessROImage(input);

            const Ren::ImageRWHandle output_tex = fg.AccessRWImage(histogram);

            const Ren::Binding bindings[] = {
                {Ren::eBindTarget::TexSampled, HistogramSample::HDR_TEX_SLOT, {input_tex, linear_sampler_}},
                {Ren::eBindTarget::ImageRW, HistogramSample::OUT_IMG_SLOT, output_tex}};

            HistogramSample::Params uniform_params = {};
            uniform_params.pre_exposure = view_state_.pre_exposure;

            DispatchCompute(fg.cmd_buf(), pi_histogram_sample_, fg.storages(), Ren::Vec3u{16, 8, 1}, bindings,
                            &uniform_params, sizeof(uniform_params), fg.descr_alloc(), fg.log());
        });
    }
    FgImgRWHandle exposure;
    { // Calc exposure
        auto &histogram_exposure = fg_builder_.AddNode("HISTOGRAM EXPOSURE");

        struct PassData {
            FgImgROHandle histogram;
            FgImgROHandle exposure_prev;
            FgImgRWHandle exposure;
        };

        auto *data = fg_builder_.AllocTempData<PassData>();
        data->histogram = histogram_exposure.AddTextureInput(histogram, Ren::eStage::ComputeShader);

        FgImgDesc params;
        params.w = params.h = 1;
        params.format = Ren::eFormat::R32F;
        params.sampling.wrap = Ren::eWrap::ClampToEdge;
        exposure = data->exposure =
            histogram_exposure.AddStorageImageOutput(EXPOSURE_TEX, params, Ren::eStage::ComputeShader);
        data->exposure_prev = histogram_exposure.AddHistoryTextureInput(exposure, Ren::eStage::ComputeShader);

        histogram_exposure.set_execute_cb([this, data, adaptation_speed](const FgContext &fg) {
            const Ren::ImageROHandle histogram_tex = fg.AccessROImage(data->histogram);
            const Ren::ImageROHandle exposure_prev_tex = fg.AccessROImage(data->exposure_prev);

            const Ren::ImageRWHandle exposure_tex = fg.AccessRWImage(data->exposure);

            const Ren::Binding bindings[] = {
                {Ren::eBindTarget::TexSampled, HistogramExposure::HISTOGRAM_TEX_SLOT, histogram_tex},
                {Ren::eBindTarget::TexSampled, HistogramExposure::EXPOSURE_PREV_TEX_SLOT, exposure_prev_tex},
                {Ren::eBindTarget::ImageRW, HistogramExposure::OUT_TEX_SLOT, exposure_tex}};

            HistogramExposure::Params uniform_params = {};
            uniform_params.min_exposure = min_exposure_;
            uniform_params.max_exposure = max_exposure_;
            uniform_params.exposure_factor = (settings.tonemap_mode != Eng::eTonemapMode::Standard) ? 1.25f : 0.5f;
            uniform_params.adaptation_speed_max = adaptation_speed[0];
            uniform_params.adaptation_speed_min = adaptation_speed[1];

            DispatchCompute(fg.cmd_buf(), pi_histogram_exposure_, fg.storages(), Ren::Vec3u{1}, bindings,
                            &uniform_params, sizeof(uniform_params), fg.descr_alloc(), fg.log());
        });
    }
    return exposure;
}

Eng::FgImgRWHandle Eng::Renderer::AddBloomPasses(const FgImgROHandle hdr_texture, const FgImgROHandle exposure_texture,
                                                 const bool compressed) {
    static const int BloomMipCount = 5;

    FgImgRWHandle downsampled[BloomMipCount];
    for (int mip = 0; mip < BloomMipCount; ++mip) {
        const std::string node_name = "BLOOM DOWNS. " + std::to_string(mip) + "->" + std::to_string(mip + 1);
        auto &bloom_downsample = fg_builder_.AddNode(node_name);

        struct PassData {
            FgImgROHandle input_tex;
            FgImgROHandle exposure_tex;
            FgImgRWHandle output_tex;
        };

        auto *data = fg_builder_.AllocTempData<PassData>();
        if (mip == 0) {
            data->input_tex = bloom_downsample.AddTextureInput(hdr_texture, Ren::eStage::ComputeShader);
        } else {
            data->input_tex = bloom_downsample.AddTextureInput(downsampled[mip - 1], Ren::eStage::ComputeShader);
        }
        data->exposure_tex = bloom_downsample.AddTextureInput(exposure_texture, Ren::eStage::ComputeShader);

        { // Image that holds downsampled bloom image
            FgImgDesc desc;
            desc.w = (view_state_.out_res[0] / 2) >> mip;
            desc.h = (view_state_.out_res[1] / 2) >> mip;
            desc.format = compressed ? Ren::eFormat::RGBA16F : Ren::eFormat::RGBA32F;
            desc.sampling.filter = Ren::eFilter::Bilinear;
            desc.sampling.wrap = Ren::eWrap::ClampToEdge;

            const std::string output_name = "Bloom Downsampled " + std::to_string(mip);
            downsampled[mip] = data->output_tex =
                bloom_downsample.AddStorageImageOutput(output_name, desc, Ren::eStage::ComputeShader);
        }

        bloom_downsample.set_execute_cb([this, data, mip, compressed](const FgContext &fg) {
            const Ren::ImageROHandle input_tex = fg.AccessROImage(data->input_tex);
            const Ren::ImageROHandle exposure_tex = fg.AccessROImage(data->exposure_tex);

            const Ren::ImageRWHandle output_tex = fg.AccessRWImage(data->output_tex);

            Bloom::Params uniform_params;
            uniform_params.img_size[0] = (view_state_.out_res[0] / 2) >> mip;
            uniform_params.img_size[1] = (view_state_.out_res[1] / 2) >> mip;
            uniform_params.pre_exposure = view_state_.pre_exposure;

            const Ren::Binding bindings[] = {
                {Ren::eBindTarget::TexSampled, Bloom::INPUT_TEX_SLOT, {input_tex, linear_sampler_}},
                {Ren::eBindTarget::TexSampled, Bloom::EXPOSURE_TEX_SLOT, exposure_tex},
                {Ren::eBindTarget::ImageRW, Bloom::OUT_IMG_SLOT, output_tex}};

            const Ren::Vec3u grp_count =
                Ren::Vec3u{(uniform_params.img_size[0] + Bloom::GRP_SIZE_X - 1u) / Bloom::GRP_SIZE_X,
                           (uniform_params.img_size[1] + Bloom::GRP_SIZE_Y - 1u) / Bloom::GRP_SIZE_Y, 1u};

            DispatchCompute(fg.cmd_buf(), pi_bloom_downsample_[compressed][mip == 0], fg.storages(), grp_count,
                            bindings, &uniform_params, sizeof(uniform_params), fg.descr_alloc(), fg.log());
        });
    }

    FgImgRWHandle upsampled[BloomMipCount - 1];
    for (int mip = BloomMipCount - 2; mip >= 0; --mip) {
        const std::string node_name = "BLOOM UPS. " + std::to_string(mip + 2) + "->" + std::to_string(mip + 1);
        auto &bloom_upsample = fg_builder_.AddNode(node_name);

        struct PassData {
            FgImgROHandle input_tex;
            FgImgROHandle blend_tex;
            FgImgRWHandle output_tex;
        };

        auto *data = fg_builder_.AllocTempData<PassData>();
        if (mip == BloomMipCount - 2) {
            data->input_tex = bloom_upsample.AddTextureInput(downsampled[mip + 1], Ren::eStage::ComputeShader);
        } else {
            data->input_tex = bloom_upsample.AddTextureInput(upsampled[mip + 1], Ren::eStage::ComputeShader);
        }
        data->blend_tex = bloom_upsample.AddTextureInput(downsampled[mip], Ren::eStage::ComputeShader);

        { // Image that holds upsampled bloom image
            FgImgDesc desc;
            desc.w = (view_state_.out_res[0] / 2) >> mip;
            desc.h = (view_state_.out_res[1] / 2) >> mip;
            desc.format = compressed ? Ren::eFormat::RGBA16F : Ren::eFormat::RGBA32F;
            desc.sampling.filter = Ren::eFilter::Bilinear;
            desc.sampling.wrap = Ren::eWrap::ClampToEdge;

            const std::string output_name = "Bloom Upsampled " + std::to_string(mip);
            upsampled[mip] = data->output_tex =
                bloom_upsample.AddStorageImageOutput(output_name, desc, Ren::eStage::ComputeShader);
        }

        bloom_upsample.set_execute_cb([this, data, mip, compressed](const FgContext &fg) {
            const Ren::ImageROHandle input_tex = fg.AccessROImage(data->input_tex);
            const Ren::ImageROHandle blend_tex = fg.AccessROImage(data->blend_tex);

            const Ren::ImageRWHandle output_tex = fg.AccessRWImage(data->output_tex);

            Bloom::Params uniform_params;
            uniform_params.img_size[0] = (view_state_.out_res[0] / 2) >> mip;
            uniform_params.img_size[1] = (view_state_.out_res[1] / 2) >> mip;
            uniform_params.blend_weight = 1.0f / float(1 + BloomMipCount - 1 - mip);

            const Ren::Binding bindings[] = {{Ren::eBindTarget::TexSampled, Bloom::INPUT_TEX_SLOT, input_tex},
                                             {Ren::eBindTarget::TexSampled, Bloom::BLEND_TEX_SLOT, blend_tex},
                                             {Ren::eBindTarget::ImageRW, Bloom::OUT_IMG_SLOT, output_tex}};

            const Ren::Vec3u grp_count =
                Ren::Vec3u{(uniform_params.img_size[0] + Bloom::GRP_SIZE_X - 1u) / Bloom::GRP_SIZE_X,
                           (uniform_params.img_size[1] + Bloom::GRP_SIZE_Y - 1u) / Bloom::GRP_SIZE_Y, 1u};

            DispatchCompute(fg.cmd_buf(), pi_bloom_upsample_[compressed], fg.storages(), grp_count, bindings,
                            &uniform_params, sizeof(uniform_params), fg.descr_alloc(), fg.log());
        });
    }

    return upsampled[0];
}

Eng::FgImgRWHandle Eng::Renderer::AddSharpenPass(const FgImgROHandle input_tex, const FgImgROHandle exposure_tex,
                                                 const bool compressed) {
    auto &sharpen = fg_builder_.AddNode("SHARPEN");

    struct PassData {
        FgImgROHandle input_tex;
        FgImgROHandle exposure_tex;
        FgImgRWHandle output_tex;
    };

    auto *data = fg_builder_.AllocTempData<PassData>();
    data->input_tex = sharpen.AddTextureInput(input_tex, Ren::eStage::ComputeShader);
    data->exposure_tex = sharpen.AddTextureInput(exposure_tex, Ren::eStage::ComputeShader);

    FgImgRWHandle output_tex;
    { // Image that holds output image
        FgImgDesc desc;
        desc.w = view_state_.out_res[0];
        desc.h = view_state_.out_res[1];
        desc.format = compressed ? Ren::eFormat::RGBA16F : Ren::eFormat::RGBA32F;
        desc.sampling.filter = Ren::eFilter::Bilinear;
        desc.sampling.wrap = Ren::eWrap::ClampToEdge;

        output_tex = data->output_tex =
            sharpen.AddStorageImageOutput("Sharpen Output", desc, Ren::eStage::ComputeShader);
    }

    sharpen.set_execute_cb([this, data, compressed](const FgContext &fg) {
        const Ren::ImageROHandle input_tex = fg.AccessROImage(data->input_tex);
        const Ren::ImageROHandle exposure_tex = fg.AccessROImage(data->exposure_tex);

        const Ren::ImageRWHandle &output_tex = fg.AccessRWImage(data->output_tex);

        Sharpen::Params uniform_params;
        uniform_params.img_size[0] = view_state_.out_res[0];
        uniform_params.img_size[1] = view_state_.out_res[1];
        uniform_params.sharpness = 0.15f; // hardcoded for now
        uniform_params.pre_exposure = view_state_.pre_exposure;

        const Ren::Binding bindings[] = {
            {Ren::eBindTarget::TexSampled, Sharpen::INPUT_TEX_SLOT, {input_tex, linear_sampler_}},
            {Ren::eBindTarget::TexSampled, Sharpen::EXPOSURE_TEX_SLOT, exposure_tex},
            {Ren::eBindTarget::ImageRW, Sharpen::OUT_IMG_SLOT, output_tex}};

        const Ren::Vec3u grp_count =
            Ren::Vec3u{(uniform_params.img_size[0] + Sharpen::GRP_SIZE_X - 1u) / Sharpen::GRP_SIZE_X,
                       (uniform_params.img_size[1] + Sharpen::GRP_SIZE_Y - 1u) / Sharpen::GRP_SIZE_Y, 1u};

        DispatchCompute(fg.cmd_buf(), pi_sharpen_[compressed], fg.storages(), grp_count, bindings, &uniform_params,
                        sizeof(uniform_params), fg.descr_alloc(), fg.log());
    });

    return output_tex;
}