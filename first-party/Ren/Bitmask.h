#pragma once

#include <type_traits>

namespace Ren {
template <class enum_type, typename = typename std::enable_if<std::is_enum<enum_type>::value>::type> class Bitmask {
    using underlying_type = typename std::underlying_type<enum_type>::type;

    static underlying_type to_mask(const enum_type e) { return 1 << static_cast<underlying_type>(e); }

    explicit Bitmask(const underlying_type mask) : mask_(mask) {}

  public:
    Bitmask() : mask_(0) {}
    Bitmask(const enum_type e) : mask_(to_mask(e)) {}

    Bitmask(const Bitmask &rhs) = default;
    Bitmask(Bitmask &&rhs) = default;

    Bitmask &operator=(const Bitmask &rhs) = default;
    Bitmask &operator=(Bitmask &&rhs) = default;

    Bitmask operator|(const enum_type rhs) const { return Bitmask(mask_ | to_mask(rhs)); }
    Bitmask operator|(const Bitmask rhs) const { return Bitmask(mask_ | rhs.mask_); }

    Bitmask operator|=(const enum_type rhs) { return (*this) = Bitmask(mask_ | to_mask(rhs)); }
    Bitmask operator|=(const Bitmask rhs) { return (*this) = Bitmask(mask_ | rhs.mask_); }

    Bitmask operator&(const enum_type rhs) const { return Bitmask(mask_ & to_mask(rhs)); }
    Bitmask operator&(const Bitmask rhs) const { return Bitmask(mask_ & rhs.mask_); }

    Bitmask operator&=(const enum_type rhs) { return (*this) = Bitmask(mask_ & to_mask(rhs)); }
    Bitmask operator&=(const Bitmask rhs) { return (*this) = Bitmask(mask_ & rhs.mask_); }

    Bitmask operator~() const { return Bitmask(~mask_); }

    bool operator==(const enum_type rhs) const { return mask_ == to_mask(rhs); }
    bool operator==(const Bitmask rhs) const { return mask_ == rhs.mask_; }

    operator bool() const { return mask_ != 0; }

  private:
    underlying_type mask_;
};
} // namespace Ren