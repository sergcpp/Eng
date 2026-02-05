#pragma once

#include <atomic>

#include "HashMap32.h"
#include "Span.h"
#include "SparseArray.h"

namespace Ren {
template <typename T, typename StorageType> class StrongRef;

template <typename T> class NamedStorage : public SparseArray<T> {
    HashMap32<String, uint32_t> items_by_name_;

  public:
    NamedStorage() = default;
    NamedStorage(const NamedStorage &rhs) = delete;

    template <class... Args> StrongRef<T, NamedStorage> Insert(Args &&...args) {
        const uint32_t index = SparseArray<T>::emplace(std::forward<Args>(args)...);

        const bool res = items_by_name_.Insert(SparseArray<T>::at(index).name(), index);
        assert(res);

        return {this, index};
    }

    void erase(const uint32_t index) {
        const String &name = SparseArray<T>::at(index).name();

        const bool res = items_by_name_.Erase(name);
        assert(res);

        SparseArray<T>::erase(index);
    }

    StrongRef<T, NamedStorage> FindByName(const std::string_view name) {
        uint32_t *p_index = items_by_name_.Find(name);
        if (p_index) {
            return {this, *p_index};
        } else {
            return {nullptr, 0};
        }
    }
};

template <typename T> class SortedStorage : public SparseArray<T> {
    std::vector<uint32_t> sorted_items_;

  public:
    SortedStorage() = default;
    SortedStorage(const SortedStorage &rhs) = delete;

    Span<const uint32_t> sorted_items() const { return sorted_items_; }

    template <class... Args> StrongRef<T, SortedStorage> Insert(Args &&...args) {
        const uint32_t index = SparseArray<T>::emplace(std::forward<Args>(args)...);

        const auto it = lower_bound(std::begin(sorted_items_), std::end(sorted_items_), index,
                                    [this](const uint32_t lhs_index, const uint32_t rhs_index) {
                                        return (*this)[lhs_index] < (*this)[rhs_index];
                                    });
        assert(it == std::end(sorted_items_) || (*this)[index] < (*this)[*it]);
        sorted_items_.insert(it, index);

        return {this, index};
    }

    void erase(const uint32_t index) {
        const auto it = lower_bound(std::begin(sorted_items_), std::end(sorted_items_), index,
                                    [this](const uint32_t lhs_index, const uint32_t rhs_index) {
                                        return (*this)[lhs_index] < (*this)[rhs_index];
                                    });
        assert(it != std::end(sorted_items_) && index == *it);
        sorted_items_.erase(it);

        SparseArray<T>::erase(index);
    }

    template <typename F> StrongRef<T, SortedStorage> LowerBound(F &&f) {
        auto first = std::begin(sorted_items_), last = std::end(sorted_items_);

        int count = int(std::distance(first, last));
        while (count > 0) {
            auto it = first;
            const int step = count / 2;
            std::advance(it, step);

            if (f((*this)[*it])) {
                first = ++it;
                count -= step + 1;
            } else {
                count = step;
            }
        }

        if (first == std::end(sorted_items_)) {
            return {};
        }

        return {this, *first};
    }

    bool CheckUnique() const {
        bool unique = true;
        for (auto it1 = std::begin(sorted_items_); it1 != std::end(sorted_items_) && unique; ++it1) {
            auto it2 = std::next(it1);
            if (it2 != std::end(sorted_items_)) {
                unique &= (*this)[*it1] < (*this)[*it2];
            }
        }
        return unique;
    }
};

class RefCounter {
  public:
    uint32_t ref_count() const { return ctrl_->strong_refs.load(); }

  protected:
    template <typename T, typename StorageType> friend class StrongRef;
    template <typename T, typename StorageType> friend class WeakRef;

    RefCounter() {
        ctrl_ = new CtrlBlock;
        ctrl_->strong_refs = ctrl_->weak_refs = 0;
    }
    RefCounter(const RefCounter &) {
        ctrl_ = new CtrlBlock;
        ctrl_->strong_refs = ctrl_->weak_refs = 0;
    }
    RefCounter &operator=(const RefCounter &) { return *this; }
    RefCounter(RefCounter &&rhs) noexcept { ctrl_ = std::exchange(rhs.ctrl_, nullptr); }
    RefCounter &operator=(RefCounter &&rhs) noexcept {
        if (ctrl_) {
            assert(ctrl_->strong_refs == 0);
            if (ctrl_->weak_refs == 0) {
                delete ctrl_;
            }
        }

        ctrl_ = std::exchange(rhs.ctrl_, nullptr);
        return (*this);
    }
    ~RefCounter() {
        if (ctrl_) {
            assert(ctrl_->strong_refs == 0);
            if (ctrl_->weak_refs == 0) {
                delete ctrl_;
            }
        }
    }

  private:
    struct CtrlBlock {
        std::atomic_uint32_t strong_refs;
        std::atomic_uint32_t weak_refs;
    };

    mutable CtrlBlock *ctrl_;
};

template <typename T, typename StorageType> class WeakRef;

template <typename T, typename StorageType> class StrongRef {
    StorageType *storage_;
    uint32_t index_;

    friend class WeakRef<T, StorageType>;

  public:
    StrongRef() : storage_(nullptr), index_(0) {}
    StrongRef(StorageType *storage, const uint32_t index) : storage_(storage), index_(index) {
        if (storage_) {
            const T &p = storage_->at(index_);
            ++p.ctrl_->strong_refs;
        }
    }
    ~StrongRef() { Release(); }

    uint32_t strong_refs() const {
        const T &p = storage_->at(index_);
        return p.ctrl_->strong_refs.load();
    }

    uint32_t weak_refs() const {
        const T &p = storage_->at(index_);
        return p.ctrl_->weak_refs.load();
    }

    StrongRef(const StrongRef &rhs) {
        storage_ = rhs.storage_;
        index_ = rhs.index_;

        if (storage_) {
            const T &p = storage_->at(index_);
            ++p.ctrl_->strong_refs;
        }
    }

    StrongRef(const WeakRef<T, StorageType> &rhs) {
        storage_ = rhs.storage_;
        index_ = rhs.index_;

        assert(storage_);
        const T &p = storage_->at(index_);
        ++p.ctrl_->strong_refs;
    }

    StrongRef(StrongRef &&rhs) noexcept {
        storage_ = std::exchange(rhs.storage_, nullptr);
        index_ = std::exchange(rhs.index_, 0);
    }

    StrongRef &operator=(const StrongRef &rhs) {
        Release();

        storage_ = rhs.storage_;
        index_ = rhs.index_;

        if (storage_) {
            const T &p = storage_->at(index_);
            ++p.ctrl_->strong_refs;
        }

        return (*this);
    }

    StrongRef &operator=(StrongRef &&rhs) noexcept {
        if (&rhs == this) {
            return (*this);
        }

        Release();

        storage_ = std::exchange(rhs.storage_, nullptr);
        index_ = std::exchange(rhs.index_, 0);

        return (*this);
    }

    T *operator->() {
        assert(storage_);
        return &storage_->at(index_);
    }

    const T *operator->() const {
        assert(storage_);
        return &storage_->at(index_);
    }

    T &operator*() {
        assert(storage_);
        return storage_->at(index_);
    }

    const T &operator*() const {
        assert(storage_);
        return storage_->at(index_);
    }

    T *get() {
        assert(storage_);
        return &storage_->at(index_);
    }

    const T *get() const {
        assert(storage_);
        return &storage_->at(index_);
    }

    explicit operator bool() const { return storage_ != nullptr; }

    uint32_t index() const { return index_; }

    bool operator==(const StrongRef &rhs) const { return storage_ == rhs.storage_ && index_ == rhs.index_; }
    bool operator!=(const StrongRef &rhs) const { return storage_ != rhs.storage_ || index_ != rhs.index_; }
    bool operator<(const StrongRef &rhs) const {
        if (storage_ < rhs.storage_) {
            return true;
        } else if (storage_ == rhs.storage_) {
            return index_ < rhs.index_;
        }
        return false;
    }

    bool operator==(const WeakRef<T, StorageType> &rhs) const {
        return storage_ == rhs.storage_ && index_ == rhs.index_;
    }
    bool operator!=(const WeakRef<T, StorageType> &rhs) const {
        return storage_ != rhs.storage_ || index_ != rhs.index_;
    }

    void Release() {
        if (storage_) {
            const T &p = storage_->at(index_);
            if (p.ctrl_->strong_refs.fetch_sub(1) == 1) {
                storage_->erase(index_);
            }
            storage_ = nullptr;
            index_ = 0;
        }
    }
};

template <typename T, typename StorageType> class WeakRef {
    StorageType *storage_;
    RefCounter::CtrlBlock *ctrl_;
    uint32_t index_;

    friend class StrongRef<T, StorageType>;

  public:
    WeakRef() : storage_(nullptr), ctrl_(nullptr), index_(0) {}
    WeakRef(StorageType *storage, const uint32_t index) : storage_(storage), ctrl_(nullptr), index_(index) {
        assert(storage);
        const T &p = storage_->at(index_);
        ctrl_ = p.ctrl_;
        ++ctrl_->weak_refs;
    }
    ~WeakRef() { Release(); }

    WeakRef(const WeakRef &rhs) {
        storage_ = rhs.storage_;
        ctrl_ = rhs.ctrl_;
        index_ = rhs.index_;

        if (ctrl_) {
            ++ctrl_->weak_refs;
        }
    }

    WeakRef(const StrongRef<T, StorageType> &rhs) {
        storage_ = rhs.storage_;
        ctrl_ = nullptr;
        index_ = rhs.index_;

        if (storage_) {
            const T &p = storage_->at(index_);
            ctrl_ = p.ctrl_;
            ++ctrl_->weak_refs;
        }
    }

    WeakRef(WeakRef &&rhs) noexcept {
        storage_ = std::exchange(rhs.storage_, nullptr);
        ctrl_ = std::exchange(rhs.ctrl_, nullptr);
        index_ = std::exchange(rhs.index_, 0);
    }

    WeakRef &operator=(const WeakRef &rhs) {
        Release();

        storage_ = rhs.storage_;
        ctrl_ = rhs.ctrl_;
        index_ = rhs.index_;

        if (ctrl_) {
            ++ctrl_->weak_refs;
        }

        return *this;
    }

    WeakRef &operator=(const StrongRef<T, StorageType> &rhs) {
        Release();

        storage_ = rhs.storage_;
        ctrl_ = nullptr;
        index_ = rhs.index_;

        if (storage_) {
            const T &p = storage_->at(index_);
            ctrl_ = p.ctrl_;
            ++ctrl_->weak_refs;
        }

        return *this;
    }

    WeakRef &operator=(WeakRef &&rhs) noexcept {
        if (&rhs == this) {
            return (*this);
        }

        Release();

        storage_ = std::exchange(rhs.storage_, nullptr);
        ctrl_ = std::exchange(rhs.ctrl_, nullptr);
        index_ = std::exchange(rhs.index_, 0);

        return *this;
    }

    T *operator->() {
        assert(storage_);
        return &storage_->at(index_);
    }

    const T *operator->() const {
        assert(storage_);
        return &storage_->at(index_);
    }

    T &operator*() {
        assert(storage_);
        return storage_->at(index_);
    }

    const T &operator*() const {
        assert(storage_);
        return storage_->at(index_);
    }

    T *get() {
        assert(storage_);
        return &storage_->at(index_);
    }

    const T *get() const {
        assert(storage_);
        return &storage_->at(index_);
    }

    explicit operator bool() const { return ctrl_ && ctrl_->strong_refs.load() != 0; }

    uint32_t index() const { return index_; }

    bool operator==(const WeakRef &rhs) const { return ctrl_ == rhs.ctrl_ && index_ == rhs.index_; }
    bool operator!=(const WeakRef &rhs) const { return ctrl_ != rhs.ctrl_ || index_ != rhs.index_; }
    bool operator<(const WeakRef &rhs) const {
        if (ctrl_ < rhs.ctrl_) {
            return true;
        } else if (ctrl_ == rhs.ctrl_) {
            return index_ < rhs.index_;
        }
        return false;
    }
    bool operator==(const StrongRef<T, StorageType> &rhs) const {
        const T &p = rhs.storage_->at(rhs.index_);
        return ctrl_ == p.ctrl_ && index_ == rhs.index_;
    }
    bool operator!=(const StrongRef<T, StorageType> &rhs) const {
        const T &p = rhs.storage_->at(rhs.index_);
        return ctrl_ != p.ctrl_ || index_ != rhs.index_;
    }

    void Release() {
        if (ctrl_) {
            if (ctrl_->weak_refs.fetch_sub(1) == 1 && ctrl_->strong_refs.load() == 0) {
                delete ctrl_;
            }
            storage_ = nullptr;
            ctrl_ = nullptr;
            index_ = 0;
        }
    }
};

} // namespace Ren

////

namespace Ren {
template <typename T, int Tag = 0> struct Handle {
    uint32_t index = 0xffffffff;
    uint32_t generation = 0;

    Handle() = default;
    Handle(const uint32_t _index, const uint32_t _generation) : index(_index), generation(_generation) {}
    explicit Handle(const uint64_t _opaque)
        : index(uint32_t(_opaque >> 32)), generation(uint32_t(_opaque & 0xffffffff)) {}

    explicit operator bool() const { return index != 0xffffffff; }
    explicit operator uint64_t() const { return (uint64_t(index) << 32) | generation; }

    bool operator==(const Handle &rhs) const { return index == rhs.index && generation == rhs.generation; }
    bool operator!=(const Handle &rhs) const { return index != rhs.index || generation != rhs.generation; }
    bool operator<(const Handle &rhs) const {
        if (index < rhs.index) {
            return true;
        } else if (index == rhs.index) {
            return generation < rhs.generation;
        }
        return false;
    }
    bool operator>(const Handle &rhs) const {
        if (index > rhs.index) {
            return true;
        } else if (index == rhs.index) {
            return generation > rhs.generation;
        }
        return false;
    }

    template <int HigherTag, typename = std::enable_if_t<(HigherTag > Tag)>> operator Handle<T, HigherTag>() const {
        return Handle<T, HigherTag>{index, generation};
    }
};

static const uint64_t InvalidHandle = 0xffffffff00000000u;

static const int RWTag = 0;
static const int ROTag = 1;

// TODO: Optimize the storage (copy SparseArray)
template <typename T, typename U> class DualStorage {
  protected:
    std::vector<T> data_main_;
    std::vector<U> data_cold_;
    mutable std::vector<uint32_t> generation_;

    std::vector<uint32_t> free_indices_;

    static_assert(std::is_default_constructible_v<T> && std::is_default_constructible_v<U>,
                  "DualStorage<T, U> requires T, U to be default constructible");

  public:
    DualStorage() = default;
    DualStorage(DualStorage &rhs) = delete;

    DualStorage &operator=(DualStorage &rhs) = delete;

    const T *data_main() const { return data_main_.data(); }
    const U *data_cold() const { return data_cold_.data(); }

    ~DualStorage() {
        assert(free_indices_.size() == data_main_.size());
        assert(free_indices_.size() == data_cold_.size());
        assert(generation_.size() == data_cold_.size());
    }

    void reserve(const size_t size) {
        data_main_.reserve(size);
        data_cold_.reserve(size);
        generation_.reserve(size);
    }

    Handle<T, RWTag> Emplace() {
        if (free_indices_.empty()) {
            const uint32_t index = uint32_t(generation_.size());
            data_main_.emplace_back();
            data_cold_.emplace_back();
            generation_.push_back(0);
            return Handle<T, RWTag>{index, 0};
        }

        const uint32_t index = free_indices_.back();
        free_indices_.pop_back();

        return Handle<T, RWTag>{index, generation_[index]};
    }

    void Free(const Handle<T, RWTag> handle) {
        assert(handle.generation == generation_[handle.index]);
        data_main_[handle.index] = {};
        data_cold_[handle.index] = {};
        ++generation_[handle.index];
        free_indices_.push_back(handle.index);
    }

    void FreeUnsafe(const uint32_t index) {
        data_main_[index] = {};
        data_cold_[index] = {};
        ++generation_[index];
        free_indices_.push_back(index);
    }

    std::pair<const T &, const U &> Get(const Handle<T, ROTag> handle) const {
        assert(handle.generation == generation_[handle.index]);
        return {data_main_[handle.index], data_cold_[handle.index]};
    }

    std::pair<T &, U &> Get(const Handle<T, RWTag> handle) {
        assert(handle.generation == generation_[handle.index]);
        return {data_main_[handle.index], data_cold_[handle.index]};
    }

    std::pair<const T &, const U &> GetUnsafe(const uint32_t index) const {
        return {data_main_[index], data_cold_[index]};
    }

    std::pair<T &, U &> GetUnsafe(const uint32_t index) { return {data_main_[index], data_cold_[index]}; }

    std::pair<const T *, const U *> TryGet(const Handle<T, ROTag> handle) const {
        if (handle.generation == generation_[handle.index]) {
            return {&data_main_[handle.index], &data_cold_[handle.index]};
        }
        return {nullptr, nullptr};
    }

    std::pair<T *, U *> TryGet(const Handle<T, RWTag> handle) {
        if (handle.generation == generation_[handle.index]) {
            return {&data_main_[handle.index], &data_cold_[handle.index]};
        }
        return {nullptr, nullptr};
    }

    void Clear() {
        data_main_.clear();
        data_cold_.clear();
        generation_.clear();
        free_indices_.clear();
    }

    uint32_t GetGeneration(const uint32_t index) const { return generation_[index]; }
    void SetGeneration(const uint32_t index, const uint32_t generation) const { generation_[index] = generation; }

    uint32_t Size() const { return uint32_t(generation_.size() - free_indices_.size()); }
    uint32_t Capacity() const { return uint32_t(generation_.capacity()); }
    bool Empty() const { return free_indices_.size() == generation_.size(); }
};

} // namespace Ren