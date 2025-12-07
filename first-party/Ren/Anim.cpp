#include "Anim.h"

#include <istream>

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable : 4996)
#endif

bool Ren::AnimSeq_Init(AnimSeqMain &anim_main, AnimSeqCold &anim_cold, Ren::String name, std::istream &data,
                       ILog *log) {
    if (!data) {
        return false;
    }

    anim_cold.name = name;

    char str[12];
    data.read(str, 12);
    assert(strcmp(str, "ANIM_SEQUEN\0") == 0);

    enum { SKELETON_CHUNK, SHAPES_CHUNK, ANIM_INFO_CHUNK, FRAMES_CHUNK };

    struct ChunkPos {
        int offset;
        int length;
    };

    struct Header {
        int num_chunks;
        ChunkPos p[4];
    } file_header = {};

    data.read((char *)&file_header.num_chunks, sizeof(int));
    data.read((char *)&file_header.p[0], file_header.num_chunks * sizeof(ChunkPos));

    const size_t bones_count = size_t(file_header.p[SKELETON_CHUNK].length) / (64 + 64 + 4);
    anim_main.bones.resize(bones_count);
    int offset = 0;
    for (size_t i = 0; i < bones_count; i++) {
        anim_main.bones[i].id = int(i);
        anim_main.bones[i].flags = 0;
        char temp_name[64];
        data.read(temp_name, 64);
        anim_main.bones[i].name = String{temp_name};
        data.read(temp_name, 64);
        anim_main.bones[i].parent_name = String{temp_name};
        int has_translate_anim = 0;
        data.read((char *)&has_translate_anim, 4);
        if (has_translate_anim) {
            anim_main.bones[i].flags |= uint32_t(eAnimBoneFlags::AnimHasTranslate);
        }
        anim_main.bones[i].offset = offset;
        if (has_translate_anim) {
            offset += 7;
        } else {
            offset += 4;
        }
    }

    if (file_header.num_chunks == 4) {
        const size_t shapes_count = size_t(file_header.p[SHAPES_CHUNK].length) / 64;
        anim_main.shapes.resize(shapes_count);
        for (size_t i = 0; i < shapes_count; i++) {
            char temp_name[64];
            data.read(temp_name, 64);
            anim_main.shapes[i].name = String{temp_name};
            anim_main.shapes[i].offset = offset;
            anim_main.shapes[i].cur_weight = 0;
            offset += 1;
        }
    }

    // support old layout
    const int AnimInfoChunk = (file_header.num_chunks == 4) ? ANIM_INFO_CHUNK : ANIM_INFO_CHUNK - 1;
    const int FramesChunk = (file_header.num_chunks == 4) ? FRAMES_CHUNK : FRAMES_CHUNK - 1;

    assert(file_header.p[AnimInfoChunk].length == 64 + 2 * sizeof(int32_t));

    anim_main.frame_size = offset;
    char act_name[64];
    data.read(act_name, 64);
    anim_cold.act_name = String{act_name};
    data.read((char *)&anim_main.fps, 4);
    data.read((char *)&anim_main.len, 4);

    anim_main.frames.resize(file_header.p[FramesChunk].length / 4);
    data.read((char *)&anim_main.frames[0], file_header.p[FramesChunk].length);

    anim_main.frame_dur = 1.0f / float(anim_main.fps);
    anim_main.anim_dur = float(anim_main.len) * anim_main.frame_dur;

    return true;
}

void Ren::AnimSeq_LinkBones(const AnimSeqMain &anim_main, const AnimSeqCold &anim_cold, Span<const Bone> bones,
                            Span<int> out_bone_indices) {
    for (int i = 0; i < int(bones.size()); i++) {
        out_bone_indices[i] = -1;
        for (int j = 0; j < int(anim_main.bones.size()); j++) {
            if (bones[i].name == anim_main.bones[j].name) {
                if (bones[i].parent_id != -1) {
                    assert(bones[bones[i].parent_id].name == anim_main.bones[j].parent_name);
                }
                out_bone_indices[i] = j;
                break;
            }
        }
    }
}

void Ren::AnimSeq_LinkShapes(const AnimSeqMain &anim_main, const AnimSeqCold &anim_cold, Span<const ShapeKey> shapes,
                             Span<int> out_shape_indices) {
    for (int i = 0; i < int(shapes.size()); i++) {
        out_shape_indices[i] = -1;
        for (int j = 0; j < int(anim_main.shapes.size()); j++) {
            if (shapes[i].name == anim_main.shapes[j].name) {
                out_shape_indices[i] = j;
                break;
            }
        }
    }
}

void Ren::AnimSeq_Update(AnimSeqMain &anim_main, AnimSeqCold &anim_cold, float time) {
    if (anim_main.len < 2) {
        return;
    }

    while (time > anim_main.anim_dur) {
        time -= anim_main.anim_dur;
    }
    while (time < 0) {
        time += anim_main.anim_dur;
    }

    const float frame = time * float(anim_main.fps);
    const float frame_fl = std::floor(frame);
    const int fr_0 = int(frame) % anim_main.len;
    const int fr_1 = int(std::ceil(frame)) % anim_main.len;
    const float t = frame - frame_fl;
    AnimSeq_InterpolateFrames(anim_main, anim_cold, fr_0, fr_1, t);
}

void Ren::AnimSeq_InterpolateFrames(AnimSeqMain &anim_main, AnimSeqCold &anim_cold, const int fr_0, const int fr_1,
                                    const float t) {
    for (AnimBone &bone : anim_main.bones) {
        int offset = bone.offset;
        if (bone.flags & uint32_t(eAnimBoneFlags::AnimHasTranslate)) {
            const Vec3f p1 = MakeVec3(&anim_main.frames[fr_0 * anim_main.frame_size + offset]);
            const Vec3f p2 = MakeVec3(&anim_main.frames[fr_1 * anim_main.frame_size + offset]);
            bone.cur_pos = Mix(p1, p2, t);
            offset += 3;
        }
        const Quatf q1 = MakeQuat(&anim_main.frames[fr_0 * anim_main.frame_size + offset]);
        const Quatf q2 = MakeQuat(&anim_main.frames[fr_1 * anim_main.frame_size + offset]);
        bone.cur_rot = Mix(q1, q2, t);
    }

    for (AnimShape &shape : anim_main.shapes) {
        const int offset = shape.offset;
        const float w1 = anim_main.frames[fr_0 * anim_main.frame_size + offset];
        const float w2 = anim_main.frames[fr_1 * anim_main.frame_size + offset];
        shape.cur_weight = Mix(w1, w2, t);
    }
}

// skeleton

Ren::Vec3f Ren::Skeleton::bone_pos(std::string_view name) const {
    const Bone *bone = find_bone(name);
    Vec3f ret;
    const float *m = ValuePtr(bone->cur_comb_matrix);
    /*ret[0] = -(m[0] * m[12] + m[1] * m[13] + m[2] * m[14]);
    ret[1] = -(m[4] * m[12] + m[5] * m[13] + m[6] * m[14]);
    ret[2] = -(m[8] * m[12] + m[9] * m[13] + m[10] * m[14]);*/

    ret[0] = m[12];
    ret[1] = m[13];
    ret[2] = m[14];

    return ret;
}

Ren::Vec3f Ren::Skeleton::bone_pos(const int i) const {
    auto bone_it = &bones[i];
    Vec3f ret;
    const float *m = ValuePtr(bone_it->cur_comb_matrix);
    /*ret[0] = -(m[0] * m[12] + m[1] * m[13] + m[2] * m[14]);
    ret[1] = -(m[4] * m[12] + m[5] * m[13] + m[6] * m[14]);
    ret[2] = -(m[8] * m[12] + m[9] * m[13] + m[10] * m[14]);*/

    ret[0] = m[12];
    ret[1] = m[13];
    ret[2] = m[14];

    return ret;
}

void Ren::Skeleton::bone_matrix(std::string_view name, Mat4f &mat) const {
    const Bone *bone = find_bone(name);
    assert(bone);
    mat = bone->cur_comb_matrix;
}

void Ren::Skeleton::bone_matrix(const int i, Mat4f &mat) const { mat = bones[i].cur_comb_matrix; }

void Ren::Skeleton::UpdateBones(Mat4f *matr_palette) {
    for (int i = 0; i < int(bones.size()); ++i) {
        if (bones[i].dirty) {
            if (bones[i].parent_id != -1) {
                bones[i].cur_comb_matrix = bones[bones[i].parent_id].cur_comb_matrix * bones[i].cur_matrix;
            } else {
                bones[i].cur_comb_matrix = bones[i].cur_matrix;
            }
            bones[i].dirty = false;
        }
        matr_palette[i] = bones[i].cur_comb_matrix * bones[i].inv_bind_matrix;
    }
}

int Ren::Skeleton::UpdateShapes(uint16_t *out_shape_palette) {
    int active_shapes_count = 0;

    for (int i = 0; i < int(shapes.size()); ++i) {
        const uint16_t weight_packed = shapes[i].cur_weight_packed;
        if (weight_packed) {
            out_shape_palette[2 * active_shapes_count + 0] = uint16_t(i);
            out_shape_palette[2 * active_shapes_count + 1] = weight_packed;
            ++active_shapes_count;
        }
    }

    return active_shapes_count;
}

int Ren::Skeleton::AddAnimSequence(const AnimSeqHandle handle,
                                   const SparseDualStorage<AnimSeqMain, AnimSeqCold> &storage) {
    for (int i = 0; i < int(anims.size()); i++) {
        if (anims[i].anim == handle) {
            return i;
        }
    }
    const auto &[anim_main, anim_cold] = storage[handle];

    AnimLink &a = anims.emplace_back();
    a.anim = handle;
    a.anim_bones.resize(bones.size());
    AnimSeq_LinkBones(anim_main, anim_cold, bones, a.anim_bones);
    a.anim_shapes.resize(bones.size());
    AnimSeq_LinkShapes(anim_main, anim_cold, shapes, a.anim_shapes);
    return int(anims.size() - 1);
}

void Ren::Skeleton::MarkChildren() {
    for (int i = 0; i < int(bones.size()); i++) {
        if (bones[i].parent_id != -1 && bones[bones[i].parent_id].dirty) {
            bones[i].dirty = true;
        }
    }
}

void Ren::Skeleton::ApplyAnim(const int id, const SparseDualStorage<AnimSeqMain, AnimSeqCold> &storage) {
    const auto &[anim_main, anim_cold] = storage[anims[id].anim];
    for (int i = 0; i < int(bones.size()); i++) {
        const int ndx = anims[id].anim_bones[i];
        if (ndx != -1) {
            const AnimBone &abone = anim_main.bones[ndx];
            Mat4f m = Mat4f{1};
            if (abone.flags & uint32_t(eAnimBoneFlags::AnimHasTranslate)) {
                m = Translate(m, abone.cur_pos);
            } else {
                m = Translate(m, bones[i].head_pos);
            }
            m *= ToMat4(abone.cur_rot);
            bones[i].cur_matrix = m;
            bones[i].dirty = true;
        }
    }
    MarkChildren();

    for (int i = 0; i < int(shapes.size()); i++) {
        const int ndx = anims[id].anim_shapes[i];
        if (ndx != -1) {
            const AnimShape &ashape = anim_main.shapes[ndx];
            shapes[i].cur_weight_packed = uint16_t(ashape.cur_weight * 65535);
        }
    }
}

void Ren::Skeleton::ApplyAnim(const int anim_id1, const int anim_id2, const float t,
                              const SparseDualStorage<AnimSeqMain, AnimSeqCold> &storage) {
    const auto &[anim1_main, anim1_cold] = storage[anims[anim_id1].anim];
    const auto &[anim2_main, anim2_cold] = storage[anims[anim_id2].anim];
    for (int i = 0; i < int(bones.size()); i++) {
        if (anims[anim_id1].anim_bones[i] != -1 || anims[anim_id2].anim_bones[i] != -1) {
            const int ndx1 = anims[anim_id1].anim_bones[i];
            const int ndx2 = anims[anim_id2].anim_bones[i];

            Mat4f m{1};
            Vec3f pos;
            Quatf orient;
            if (ndx1 != -1) {
                const AnimBone &abone1 = anim1_main.bones[ndx1];
                if (abone1.flags & uint32_t(eAnimBoneFlags::AnimHasTranslate)) {
                    pos = abone1.cur_pos;
                } else {
                    pos = bones[i].head_pos;
                }
                orient = abone1.cur_rot;
            }
            if (ndx2 != -1) {
                const AnimBone &abone2 = anim2_main.bones[anims[anim_id2].anim_bones[i]];
                if (abone2.flags & uint32_t(eAnimBoneFlags::AnimHasTranslate)) {
                    pos = Mix(pos, abone2.cur_pos, t);
                }
                orient = Slerp(orient, abone2.cur_rot, t);
            }
            m = Translate(m, pos);
            m *= ToMat4(orient);
            bones[i].cur_matrix = m;
            bones[i].dirty = true;
        }
    }
    MarkChildren();
}

void Ren::Skeleton::UpdateAnim(const int anim_id, const float t, SparseDualStorage<AnimSeqMain, AnimSeqCold> &storage) {
    const auto &[anim_main, anim_cold] = storage[anims[anim_id].anim];
    AnimSeq_Update(anim_main, anim_cold, t);
}

#ifdef _MSC_VER
#pragma warning(pop)
#endif
