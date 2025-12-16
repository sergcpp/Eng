#include "DescriptorPool.h"

#include "GLCtx.h"
#include "SmallVector.h"

Ren::DescrPool &Ren::DescrPool::operator=(DescrPool &&rhs) noexcept {
    if (this == &rhs) {
        return (*this);
    }

    Destroy();

    sets_count_ = std::exchange(rhs.sets_count_, 0);
    next_free_ = std::exchange(rhs.next_free_, 0);
    for (int i = 0; i < int(eDescrType::_Count); ++i) {
        descr_counts_[i] = rhs.descr_counts_[i];
    }

    return (*this);
}

bool Ren::DescrPool::Init(const DescrSizes &sizes, const uint32_t sets_count) {
    Destroy();

    descr_counts_[int(eDescrType::CombinedImageSampler)] = sizes.img_sampler_count;
    descr_counts_[int(eDescrType::SampledImage)] = sizes.img_count;
    descr_counts_[int(eDescrType::Sampler)] = sizes.sampler_count;
    descr_counts_[int(eDescrType::StorageImage)] = sizes.store_img_count;
    descr_counts_[int(eDescrType::UniformBuffer)] = sizes.ubuf_count;
    descr_counts_[int(eDescrType::UniformTexBuffer)] = sizes.utbuf_count;
    descr_counts_[int(eDescrType::StorageBuffer)] = sizes.sbuf_count;
    descr_counts_[int(eDescrType::StorageTexBuffer)] = sizes.stbuf_count;

    return true;
}

void Ren::DescrPool::Destroy() {}

/////////////////////////////////////////////////////////////////////////////////////////////////

Ren::DescrMultiPoolAlloc::DescrMultiPoolAlloc(const ApiContext &api, const uint32_t pool_step,
                                              const uint32_t max_img_sampler_count, const uint32_t max_img_count,
                                              const uint32_t max_sampler_count, const uint32_t max_store_img_count,
                                              const uint32_t max_ubuf_count, const uint32_t max_tbuf_count,
                                              const uint32_t max_sbuf_count, const uint32_t max_stbuf_count,
                                              const uint32_t max_acc_count, const uint32_t initial_sets_count)
    : pool_step_(pool_step) {
    img_sampler_based_count_ = (max_img_sampler_count + pool_step - 1) / pool_step;
    img_based_count_ = (max_img_count + pool_step - 1) / pool_step;
    sampler_based_count_ = (max_sampler_count + pool_step - 1) / pool_step;
    store_img_based_count_ = (max_store_img_count + pool_step - 1) / pool_step;
    ubuf_based_count_ = (max_ubuf_count + pool_step - 1) / pool_step;
    utbuf_based_count_ = (max_tbuf_count + pool_step - 1) / pool_step;
    sbuf_based_count_ = (max_sbuf_count + pool_step - 1) / pool_step;
    stbuf_based_count_ = (max_stbuf_count + pool_step - 1) / pool_step;
    acc_based_count_ = (max_acc_count + pool_step - 1) / pool_step;
    const uint32_t required_pools_count = img_sampler_based_count_ * img_based_count_ * sampler_based_count_ *
                                          store_img_based_count_ * ubuf_based_count_ * utbuf_based_count_ *
                                          sbuf_based_count_ * stbuf_based_count_ * std::max(acc_based_count_, 1u);

    // store rounded values
    max_img_sampler_count_ = pool_step * img_sampler_based_count_;
    max_img_count_ = pool_step * img_based_count_;
    max_sampler_count_ = pool_step * sampler_based_count_;
    max_store_img_count_ = pool_step * store_img_based_count_;
    max_ubuf_count_ = pool_step * ubuf_based_count_;
    max_utbuf_count_ = pool_step * utbuf_based_count_;
    max_sbuf_count_ = pool_step * sbuf_based_count_;
    max_stbuf_count_ = pool_step * stbuf_based_count_;
    max_acc_count_ = pool_step * acc_based_count_;

    for (uint32_t i = 0; i < required_pools_count; ++i) {
        uint32_t index = i;

        DescrSizes pool_sizes = {};
        pool_sizes.acc_count = pool_step * ((index % acc_based_count_) + 1);
        index /= acc_based_count_;
        pool_sizes.stbuf_count = pool_step * ((index % stbuf_based_count_) + 1);
        index /= stbuf_based_count_;
        pool_sizes.sbuf_count = pool_step * ((index % sbuf_based_count_) + 1);
        index /= sbuf_based_count_;
        pool_sizes.utbuf_count = pool_step * ((index % utbuf_based_count_) + 1);
        index /= utbuf_based_count_;
        pool_sizes.ubuf_count = pool_step * ((index % ubuf_based_count_) + 1);
        index /= ubuf_based_count_;
        pool_sizes.store_img_count = pool_step * ((index % store_img_based_count_) + 1);
        index /= store_img_based_count_;
        pool_sizes.sampler_count = pool_step * ((index % sampler_based_count_) + 1);
        index /= sampler_based_count_;
        pool_sizes.img_count = pool_step * ((index % img_based_count_) + 1);
        index /= img_based_count_;
        pool_sizes.img_sampler_count = pool_step * ((index % img_sampler_based_count_) + 1);
        index /= img_sampler_based_count_;

        pools_.emplace_back(api, pool_sizes, initial_sets_count);
    }
    assert(pools_.size() == required_pools_count);
}
