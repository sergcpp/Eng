#pragma once

#include <cstdint>
#include <cstring>

#include <iosfwd>
#include <vector>

#include "Fwd.h"
#include "math/Mat.h"
#include "math/Quat.h"
#include "utils/Span.h"
#include "utils/String.h"

namespace Ren {
enum class eAnimBoneFlags { AnimHasTranslate = 1 };

struct AnimBone {
    String name;
    String parent_name;
    int id = -1;
    int offset = 0;
    uint32_t flags = 0;
    Vec3f cur_pos;
    Quatf cur_rot;
};

struct AnimShape {
    String name;
    int offset = 0;
    float cur_weight = 0;
};

struct Bone;
struct ShapeKey;

struct AnimSeqMain {
    int fps = 0;
    int len = 0;
    int frame_size = 0;
    float frame_dur = 0;
    float anim_dur = 0;
    std::vector<float> frames;
    std::vector<AnimBone> bones;
    std::vector<AnimShape> shapes;
};

struct AnimSeqCold {
    String name, act_name;
};

bool AnimSeq_Init(AnimSeqMain &anim_main, AnimSeqCold &anim_cold, Ren::String name, std::istream &data, ILog *log);

void AnimSeq_LinkBones(const AnimSeqMain &anim_main, const AnimSeqCold &anim_cold, Span<const Bone> bones,
                       Span<int> out_bone_indices);
void AnimSeq_LinkShapes(const AnimSeqMain &anim_main, const AnimSeqCold &anim_cold, Span<const ShapeKey> shapes,
                        Span<int> out_shape_indices);

void AnimSeq_Update(AnimSeqMain &anim_main, AnimSeqCold &anim_cold, float time);
void AnimSeq_InterpolateFrames(AnimSeqMain &anim_main, AnimSeqCold &anim_cold, int fr_0, int fr_1, float t);

struct AnimLink {
    AnimSeqHandle anim;
    std::vector<int> anim_bones;
    std::vector<int> anim_shapes;
};

struct Bone {
    String name;
    int id = -1;
    int parent_id = -1;
    bool dirty = false;
    Mat4f cur_matrix;
    Mat4f cur_comb_matrix;
    Mat4f bind_matrix;
    Mat4f inv_bind_matrix;
    Vec3f head_pos;
};

struct ShapeKey {
    String name;
    uint32_t delta_offset, delta_count;
    uint16_t cur_weight_packed;
};

struct Skeleton {
    std::vector<Bone> bones;
    std::vector<ShapeKey> shapes;
    std::vector<AnimLink> anims;

    [[nodiscard]] const Bone *find_bone(std::string_view name) const {
        for (int i = 0; i < int(bones.size()); i++) {
            if (name == bones[i].name) {
                return &bones[i];
            }
        }
        return nullptr;
    }

    [[nodiscard]] Vec3f bone_pos(std::string_view name) const;
    [[nodiscard]] Vec3f bone_pos(int i) const;

    void bone_matrix(std::string_view name, Mat4f &mat) const;
    void bone_matrix(int i, Mat4f &mat) const;

    int AddAnimSequence(AnimSeqHandle handle, const SparseDualStorage<AnimSeqMain, AnimSeqCold> &storage);

    void MarkChildren();
    void ApplyAnim(int id, const SparseDualStorage<AnimSeqMain, AnimSeqCold> &storage);
    void ApplyAnim(int anim_id1, int anim_id2, float t, const SparseDualStorage<AnimSeqMain, AnimSeqCold> &storage);
    void UpdateAnim(int anim_id, float t, SparseDualStorage<AnimSeqMain, AnimSeqCold> &storage);
    void UpdateBones(Mat4f *matr_palette);
    int UpdateShapes(uint16_t *out_shape_palette);
};
} // namespace Ren