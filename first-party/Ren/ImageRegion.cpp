#include "ImageRegion.h"

#include "ApiContext.h"
#include "ImageAtlas.h"
#include "utils/Utils.h"

Ren::ImageRegion::ImageRegion(std::string_view name, ImageAtlasArray *atlas, const int pos[3])
    : name_(name), atlas_(atlas) {
    memcpy(pos_, pos, 3 * sizeof(int));
}

Ren::ImageRegion::ImageRegion(std::string_view name, Span<const uint8_t> data, const ImgParams &p,
                              CommandBuffer cmd_buf, ImageAtlasArray *atlas, eImgLoadStatus *load_status, ILog *log)
    : name_(name) {
    Init(data, p, cmd_buf, atlas, load_status, log);
}

Ren::ImageRegion::ImageRegion(std::string_view name, const BufferMain &sbuf, int data_off, int data_len,
                              const ImgParams &p, CommandBuffer cmd_buf, ImageAtlasArray *atlas,
                              eImgLoadStatus *load_status, ILog *log)
    : name_(name) {
    Init(sbuf, data_off, data_len, p, cmd_buf, atlas, load_status, log);
}

Ren::ImageRegion::~ImageRegion() {
    if (atlas_) {
        atlas_->Free(pos_);
    }
}

Ren::ImageRegion &Ren::ImageRegion::operator=(ImageRegion &&rhs) noexcept {
    RefCounter::operator=(std::move(rhs));

    if (atlas_) {
        atlas_->Free(pos_);
    }

    name_ = std::move(rhs.name_);
    atlas_ = std::exchange(rhs.atlas_, nullptr);
    memcpy(pos_, rhs.pos_, 3 * sizeof(int));
    params = rhs.params;
    ready_ = std::exchange(rhs.ready_, false);

    return (*this);
}

void Ren::ImageRegion::Init(Span<const uint8_t> data, const ImgParams &p, CommandBuffer _cmd_buf,
                            ImageAtlasArray *atlas, eImgLoadStatus *load_status, ILog *log) {
    if (data.empty()) {
        BufferMain stage_buf_main = {};
        BufferCold stage_buf_cold = {};
        if (!Buffer_Init(*atlas->api(), stage_buf_main, stage_buf_cold, String{"Temp upload buf"}, eBufType::Upload, 4,
                         log)) {
            log->Error("Failed to initialize temp upload buffer");
            return;
        }

        { // Update staging buffer
            uint8_t *out_col = Buffer_Map(*atlas->api(), stage_buf_main, stage_buf_cold);
            out_col[0] = 0;
            out_col[1] = out_col[2] = out_col[3] = 255;
            Buffer_Unmap(*atlas->api(), stage_buf_main, stage_buf_cold);
        }

        CommandBuffer cmd_buf = _cmd_buf;
        if (!cmd_buf) {
            cmd_buf = atlas->api()->BegSingleTimeCommands();
        }

        ImgParams _p;
        _p.w = _p.h = 1;
        _p.format = eFormat::RGBA8;
        [[maybe_unused]] const bool res = InitFromRAWData(stage_buf_main, 0, 4, cmd_buf, _p, atlas);

        if (!_cmd_buf) {
            atlas->api()->EndSingleTimeCommands(cmd_buf);
            Buffer_DestroyImmediately(*atlas->api(), stage_buf_main, stage_buf_cold);
        }

        // mark it as not ready
        ready_ = false;
        (*load_status) = eImgLoadStatus::CreatedDefault;
    } else {
        if (atlas_) {
            atlas_->Free(pos_);
        }

        if (name_.EndsWith(".dds") != 0 || name_.EndsWith(".DDS") != 0) {
            ready_ = InitFromDDSFile(data, p, atlas, log);
        } else {
            BufferMain stage_buf_main = {};
            BufferCold stage_buf_cold = {};
            if (!Buffer_Init(*atlas->api(), stage_buf_main, stage_buf_cold, String{"Temp upload buf"}, eBufType::Upload,
                             uint32_t(data.size()), log)) {
                log->Error("Failed to initialize temp upload buffer");
                return;
            }

            { // Update staging buffer
                uint8_t *stage_data = Buffer_Map(*atlas->api(), stage_buf_main, stage_buf_cold);
                memcpy(stage_data, data.data(), data.size());
                Buffer_Unmap(*atlas->api(), stage_buf_main, stage_buf_cold);
            }

            CommandBuffer cmd_buf = _cmd_buf;
            if (!cmd_buf) {
                cmd_buf = atlas->api()->BegSingleTimeCommands();
            }
            ready_ = InitFromRAWData(stage_buf_main, 0, int(data.size()), cmd_buf, p, atlas);
            if (!_cmd_buf) {
                atlas->api()->EndSingleTimeCommands(cmd_buf);
                Buffer_DestroyImmediately(*atlas->api(), stage_buf_main, stage_buf_cold);
            }
        }
        (*load_status) = ready_ ? eImgLoadStatus::CreatedFromData : eImgLoadStatus::Error;
    }
}

void Ren::ImageRegion::Init(const BufferMain &sbuf, int data_off, int data_len, const ImgParams &p,
                            CommandBuffer cmd_buf, ImageAtlasArray *atlas, eImgLoadStatus *load_status, ILog *log) {
    if (atlas_) {
        atlas_->Free(pos_);
    }
    ready_ = InitFromRAWData(sbuf, data_off, data_len, cmd_buf, p, atlas);
    (*load_status) = ready_ ? eImgLoadStatus::CreatedFromData : eImgLoadStatus::Error;
}

bool Ren::ImageRegion::InitFromDDSFile(Span<const uint8_t> data, ImgParams p, ImageAtlasArray *atlas, Ren::ILog *log) {
    uint32_t offset = 0;
    if (data.size() - offset < sizeof(DDSHeader)) {
        return false;
    }

    DDSHeader header;
    memcpy(&header, &data[offset], sizeof(DDSHeader));
    offset += sizeof(DDSHeader);

    ParseDDSHeader(header, &p);
    if (header.sPixelFormat.dwFourCC == Ren::FourCC_DX10) {
        if (data.size() - offset < sizeof(DDS_HEADER_DXT10)) {
            return false;
        }

        DDS_HEADER_DXT10 dx10_header = {};
        memcpy(&dx10_header, &data[offset], sizeof(DDS_HEADER_DXT10));
        offset += sizeof(DDS_HEADER_DXT10);

        p.format = FormatFromDXGIFormat(dx10_header.dxgiFormat);
    }

    const int img_data_len = GetDataLenBytes(p.w, p.h, p.format);

    if (data.size() - offset < img_data_len) {
        return false;
    }

    BufferMain stage_buf_main = {};
    BufferCold stage_buf_cold = {};
    if (!Buffer_Init(*atlas->api(), stage_buf_main, stage_buf_cold, Ren::String{"Temp Stage Buf"},
                     Ren::eBufType::Upload, uint32_t(img_data_len), log)) {
        return false;
    }

    { // update staging buffer
        uint8_t *img_stage = Buffer_Map(*atlas->api(), stage_buf_main, stage_buf_cold);
        memcpy(img_stage, &data[offset], img_data_len);
        Buffer_Unmap(*atlas->api(), stage_buf_main, stage_buf_cold);
    }

    CommandBuffer cmd_buf = atlas->api()->BegSingleTimeCommands();
    const bool res = InitFromRAWData(stage_buf_main, 0, img_data_len, cmd_buf, p, atlas);
    atlas->api()->EndSingleTimeCommands(cmd_buf);

    Buffer_DestroyImmediately(*atlas->api(), stage_buf_main, stage_buf_cold);

    return res;
}

bool Ren::ImageRegion::InitFromRAWData(const BufferMain &sbuf, const int data_off, const int data_len,
                                       CommandBuffer cmd_buf, const ImgParams &p, ImageAtlasArray *atlas) {
    const int res[2] = {p.w, p.h};
    const int node = atlas->Allocate(sbuf, data_off, data_len, cmd_buf, p.format, res, pos_, 1);
    if (node == -1) {
        return false;
    }
    atlas_ = atlas;
    params = p;
    return true;
}