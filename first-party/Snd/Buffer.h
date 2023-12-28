#pragma once

#include <cstdint>

#include "Storage.h"
#include "String.h"

namespace Snd {
class ILog;
    
enum class eBufFormat { Undefined, Mono8, Mono16, Stereo8, Stereo16, Count };

struct BufParams {
    eBufFormat format = eBufFormat::Undefined;
    uint32_t samples_per_sec = 0;
};

enum class eBufLoadStatus { Found, CreatedDefault, CreatedFromData };

class Buffer : public RefCounter {
    String name_;
#ifdef USE_AL_SOUND
    uint32_t buf_id_ = 0xffffffff;
#endif
    uint32_t size_ = 0;
    BufParams params_;
    
    void FreeBuf();
  public:
    Buffer(const char *name, const void *data, uint32_t size, const BufParams &params,
           eBufLoadStatus *load_status, ILog *log);
    ~Buffer();

    Buffer(const Buffer &rhs) = delete;
    Buffer(Buffer &&rhs) noexcept;

    Buffer &operator=(const Buffer &rhs) = delete;
    Buffer &operator=(Buffer &&rhs) noexcept;

    const String &name() const { return name_; }

    bool ready() const { return params_.format != eBufFormat::Undefined; }

#ifdef USE_AL_SOUND
    uint32_t id() const { return buf_id_; }
#endif

    float GetDurationS() const;

    void SetData(const void *data, uint32_t size, const BufParams &params);

    void Init(const void *data, uint32_t size, const BufParams &params,
              eBufLoadStatus *load_status, ILog *log);
};

typedef StrongRef<Buffer> BufferRef;
typedef Storage<Buffer> BufferStorage;
} // namespace Snd
