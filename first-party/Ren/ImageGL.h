#pragma once

#include <cstdint>
#include <cstring>

#include "Buffer.h"
#include "ImageParams.h"
#include "MVec.h"

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable : 4996)
#endif

namespace Ren {
class ILog;

struct ImageMain {
    uint32_t img = 0;
    SmallVector<uint32_t, 1> views;
    uint32_t generation = 0;
    mutable eResState resource_state = eResState::Undefined;
};

struct ImageCold {
    String name;
    MemAllocation alloc;
    ImgParams params;
};

bool Image_Init(const ApiContext &api, ImageMain &img_main, ImageCold &img_cold, String name, const ImgParams &p,
                const BufferMain *sbuf_main, const BufferCold *sbuf_cold, uint32_t data_off, CommandBuffer cmd_buf,
                MemAllocators *mem_allocs, ILog *log);
bool Image_Init(const ApiContext &api, ImageCold &img_cold, String name, const ImgParams &p, MemAllocation &&alloc,
                ILog *log);

void Image_Destroy(const ApiContext &api, ImageMain &img_main, ImageCold &img_cold);
void Image_DestroyImmediately(const ApiContext &api, ImageMain &img_main, ImageCold &img_cold);

bool Image_SetSampling(const ApiContext &api, ImageMain &img_main, ImageCold &img_cold, SamplingParams s, ILog *log);

void Image_SetSubImage(const ApiContext &api, ImageMain &img_main, ImageCold &img_cold, int layer, int level,
                       const Vec3i &offset, const Vec3i &size, eFormat format, const BufferMain &sbuf_main,
                       CommandBuffer cmd_buf, int data_off, int data_len);

int Image_AddView(const ApiContext &api, ImageMain &img_main, ImageCold &img_cold, eFormat format, int mip_level,
                  int mip_count, int base_layer, int layer_count);

void Image_CmdClear(const ApiContext &api, ImageMain &img_main, ImageCold &img_cold, const ClearColor &col,
                    CommandBuffer cmd_buf);
void Image_CmdCopyToBuffer(const ApiContext &api, const ImageMain &img_main, const ImageCold &img_cold,
                           BufferMain &buf_main, BufferCold &buf_cold, CommandBuffer cmd_buf, uint32_t data_off);
void Image_CmdCopyToImage(const ApiContext &api, CommandBuffer cmd_buf, const ImageMain &src_main,
                          const ImageCold &src_cold, uint32_t src_level, uint32_t src_x, uint32_t src_y, uint32_t src_z,
                          const ImageMain &dst_main, const ImageCold &dst_cold, uint32_t dst_level, uint32_t dst_x,
                          uint32_t dst_y, uint32_t dst_z, uint32_t dst_face, uint32_t w, uint32_t h, uint32_t d);

struct ImgHandle {
    uint32_t id = 0; // native gl name
    SmallVector<uint32_t, 1> views;
    uint32_t generation = 0; // used to identify unique texture (name can be reused)

    ImgHandle() = default;
    ImgHandle(const uint32_t _id, const uint32_t _gen) : id(_id), generation(_gen) {}

    explicit operator bool() const { return id != 0; }
};
static_assert(sizeof(ImgHandle) == 40);

inline bool operator==(const ImgHandle lhs, const ImgHandle rhs) {
    return lhs.id == rhs.id && lhs.views == rhs.views && lhs.generation == rhs.generation;
}
inline bool operator!=(const ImgHandle lhs, const ImgHandle rhs) { return !operator==(lhs, rhs); }
inline bool operator<(const ImgHandle lhs, const ImgHandle rhs) {
    if (lhs.id < rhs.id) {
        return true;
    } else if (lhs.id == rhs.id) {
        if (lhs.views < rhs.views) {
            return true;
        } else if (lhs.views == rhs.views) {
            return lhs.generation < rhs.generation;
        }
    }
    return false;
}

class MemAllocators;

class Image : public RefCounter {
    ApiContext *api_ = nullptr;
    ImgHandle handle_;
    String name_;

    void InitFromRAWData(const BufferMain *sbuf_main, const BufferCold *sbuf_cold, int data_off, const ImgParams &p,
                         ILog *log);
    void InitFromRAWData(const BufferMain *sbuf_main, const BufferCold *sbuf_cold, int data_off[6], const ImgParams &p,
                         ILog *log);

  public:
    ImgParams params;

    uint32_t first_user = 0xffffffff;
    mutable eResState resource_state = eResState::Undefined;

    Image() = default;
    Image(std::string_view name, ApiContext *api, const ImgParams &p, MemAllocators *mem_allocs, ILog *log);
    Image(std::string_view name, ApiContext *api, const ImgHandle &handle, const ImgParams &p, MemAllocation &&alloc,
          ILog *log)
        : api_(api), name_(name) {
        Init(handle, p, std::move(alloc), log);
    }
    Image(std::string_view name, ApiContext *api, Span<const uint8_t> data, const ImgParams &p,
          MemAllocators *mem_allocs, eImgLoadStatus *load_status, ILog *log);
    Image(std::string_view name, ApiContext *api, Span<const uint8_t> data[6], const ImgParams &p,
          MemAllocators *mem_allocs, eImgLoadStatus *load_status, ILog *log);
    Image(const Image &rhs) = delete;
    Image(Image &&rhs) noexcept { (*this) = std::move(rhs); }
    ~Image();

    void Free();
    void FreeImmediate() { Free(); }

    Image &operator=(const Image &rhs) = delete;
    Image &operator=(Image &&rhs) noexcept;

    uint64_t GetBindlessHandle() const;

    void Init(const ImgParams &p, MemAllocators *mem_allocs, ILog *log);
    void Init(const ImgHandle &handle, const ImgParams &_params, MemAllocation &&alloc, ILog *log);
    void Init(Span<const uint8_t> data, const ImgParams &p, MemAllocators *mem_allocs, eImgLoadStatus *load_status,
              ILog *log);
    void Init(Span<const uint8_t> data[6], const ImgParams &p, MemAllocators *mem_allocs, eImgLoadStatus *load_status,
              ILog *log);

    void Realloc(int w, int h, int mip_count, int samples, eFormat format, CommandBuffer cmd_buf,
                 MemAllocators *mem_allocs, ILog *log);

    [[nodiscard]] ImgHandle handle() const { return handle_; }
    [[nodiscard]] uint32_t id() const { return handle_.id; }
    [[nodiscard]] uint32_t generation() const { return handle_.generation; }
    [[nodiscard]] const MemAllocation &mem_alloc() const {
        static MemAllocation dummy;
        return dummy;
    }
    [[nodiscard]] const String &name() const { return name_; }

    void SetSampling(SamplingParams sampling) { params.sampling = sampling; }
    void ApplySampling(SamplingParams sampling, ILog *log);

    int AddView(eFormat format, int mip_level, int mip_count, int base_layer, int layer_count);

    void SetSubImage(int layer, int level, int offsetx, int offsety, int offsetz, int sizex, int sizey, int sizez,
                     eFormat format, const BufferMain &sbuf_main, CommandBuffer cmd_buf, int data_off,
                     int data_len) const;
    void SetSubImage(int level, int offsetx, int offsety, int offsetz, int sizex, int sizey, int sizez, eFormat format,
                     const BufferMain &sbuf_main, CommandBuffer cmd_buf, int data_off, int data_len) const {
        SetSubImage(0, level, offsetx, offsety, offsetz, sizex, sizey, sizez, format, sbuf_main, cmd_buf, data_off,
                    data_len);
    }
    void SetSubImage(int offsetx, int offsety, int offsetz, int sizex, int sizey, int sizez, eFormat format,
                     const BufferMain &sbuf_main, CommandBuffer cmd_buf, int data_off, int data_len) const {
        SetSubImage(0, 0, offsetx, offsety, offsetz, sizex, sizey, sizez, format, sbuf_main, cmd_buf, data_off,
                    data_len);
    }
    void SetSubImage(int offsetx, int offsety, int sizex, int sizey, eFormat format, const BufferMain &sbuf_main,
                     CommandBuffer cmd_buf, int data_off, int data_len) const {
        SetSubImage(0, 0, offsetx, offsety, 0, sizex, sizey, 1, format, sbuf_main, cmd_buf, data_off, data_len);
    }

    void CopyTextureData(const BufferMain &sbuf_main, const BufferCold &sbuf_cold, CommandBuffer cmd_buf, int data_off,
                         int data_len) const;
};

void CopyImageToImage(CommandBuffer cmd_buf, const Image &src, uint32_t src_level, uint32_t src_x, uint32_t src_y,
                      uint32_t src_z, const Image &dst, uint32_t dst_level, uint32_t dst_x, uint32_t dst_y,
                      uint32_t dst_z, uint32_t dst_face, uint32_t w, uint32_t h, uint32_t d);

void ClearImage(const Image &tex, const ClearColor &col, CommandBuffer cmd_buf);

uint32_t GLFormatFromFormat(eFormat format);
uint32_t GLInternalFormatFromFormat(eFormat format);
uint32_t GLTypeFromFormat(eFormat format);

void GLUnbindTextureUnits(int start, int count);
} // namespace Ren

#ifdef _MSC_VER
#pragma warning(pop)
#endif
