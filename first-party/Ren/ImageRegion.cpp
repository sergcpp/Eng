#include "ImageRegion.h"

#include "ApiContext.h"
#include "ImageAtlas.h"
#include "utils/Utils.h"

bool Ren::ImageRegion_Init(ImageRegionMain &reg_main, ImageRegionCold &reg_cold, String name, Span<const uint8_t> data,
                           const ImgParams &p, CommandBuffer _cmd_buf, ImageAtlasArray *atlas, ILog *log) {
    if (name.EndsWith(".dds") != 0 || name.EndsWith(".DDS") != 0) {
        return ImageRegion_InitFromDDSFile(reg_main, reg_cold, name, data, p, atlas, log);
    } else {
        BufferMain stage_buf_main = {};
        BufferCold stage_buf_cold = {};
        if (!Buffer_Init(*atlas->api(), stage_buf_main, stage_buf_cold, String{"Temp upload buf"}, eBufType::Upload,
                         uint32_t(data.size()), log)) {
            log->Error("Failed to initialize temp upload buffer");
            return false;
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
        const bool ret = ImageRegion_InitFromRAWData(reg_main, reg_cold, name, stage_buf_main, 0, int(data.size()),
                                                     cmd_buf, p, atlas);
        if (!_cmd_buf) {
            atlas->api()->EndSingleTimeCommands(cmd_buf);
            Buffer_DestroyImmediately(*atlas->api(), stage_buf_main, stage_buf_cold);
        }
        return ret;
    }
    return false;
}

bool Ren::ImageRegion_Init(ImageRegionMain &reg_main, ImageRegionCold &reg_cold, String name, const BufferMain &sbuf,
                           int data_off, int data_len, const ImgParams &p, CommandBuffer cmd_buf,
                           ImageAtlasArray *atlas, ILog *log) {
    return ImageRegion_InitFromRAWData(reg_main, reg_cold, name, sbuf, data_off, data_len, cmd_buf, p, atlas);
}

bool Ren::ImageRegion_InitFromDDSFile(ImageRegionMain &reg_main, ImageRegionCold &reg_cold, String name,
                                      Span<const uint8_t> data, ImgParams p, ImageAtlasArray *atlas, ILog *log) {
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
    const bool res =
        ImageRegion_InitFromRAWData(reg_main, reg_cold, name, stage_buf_main, 0, img_data_len, cmd_buf, p, atlas);
    atlas->api()->EndSingleTimeCommands(cmd_buf);

    Buffer_DestroyImmediately(*atlas->api(), stage_buf_main, stage_buf_cold);

    return res;
}

bool Ren::ImageRegion_InitFromRAWData(ImageRegionMain &reg_main, ImageRegionCold &reg_cold, String name,
                                      const BufferMain &sbuf, int data_off, int data_len, CommandBuffer cmd_buf,
                                      const ImgParams &p, ImageAtlasArray *atlas) {
    const int res[2] = {p.w, p.h};
    const int node = atlas->Allocate(sbuf, data_off, data_len, cmd_buf, p.format, res, reg_main.pos, 1);
    if (node == -1) {
        return false;
    }
    reg_main.atlas = atlas;
    reg_cold.name = name;
    reg_cold.params = p;
    return true;
}
