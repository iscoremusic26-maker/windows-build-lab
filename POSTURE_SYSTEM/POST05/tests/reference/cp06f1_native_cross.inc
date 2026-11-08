// Reference transcription (NOT production code) of the engine predicates FIX01 relies on.
// Source: TwilitRealm/dusklight 40457c6adb381928e4b5fef6ed459ed291edd5e2
//   src/SSystem/SComponent/c_m3d.cpp:34-38    cM3d_Len2dSq
//   src/SSystem/SComponent/c_m3d.cpp:1143-1161 cM3d_Cross_CylSph(const cM3dGCyl*, const cM3dGSph*, f32*)
// Only the parameter types are replaced by the lab's plain structs; arithmetic order is kept.
inline float ref_cM3d_Len2dSq(float pX1,float pY1,float pX2,float pY2){
    float xDiff = pX1 - pX2;
    float yDiff = pY1 - pY2;
    return xDiff * xDiff + yDiff * yDiff;
}
inline bool ref_cM3d_Cross_CylSph(const cp06f1::Cyl* cyl,const cp06f1::Sph* sph,float* param_2){
    const cp06f1::V3* sph_center = &sph->c;
    const cp06f1::V3* cyl_center = &cyl->c;
    float radius_sum = cyl->r + sph->r;
    float dist = std::sqrt(ref_cM3d_Len2dSq(sph_center->x, sph_center->z, cyl_center->x, cyl_center->z));

    if (radius_sum < dist) {
        return false;
    }

    if (sph_center->y + sph->r >= cyl_center->y
        && sph_center->y - sph->r <= cyl_center->y + cyl->h)
    {
        *param_2 = radius_sum - dist;
        return true;
    }

    return false;
}
// d_a_e_oc.cpp:1379-1392 executeAttack lunge (frame_ctrl truncation toward zero) and
// d_a_e_oc.cpp:1452-1455 AT window on the PRE-play frame.
inline float ref_lunge(bool b,float frame){
    float my_float = 0.0f;
    int frame_ctrl = (frame - 9.0f);
    if (frame_ctrl >= 0) {
        if (frame_ctrl >= 9) {
            frame_ctrl = 9;
        }
        my_float = b ? cp06f1::kLungeB[frame_ctrl] : cp06f1::kLungeC[frame_ctrl];
    }
    return my_float;
}
inline bool ref_at_on(float pre){return pre >= 14.0f && pre <= 22.0f;}
// J3DAnimation.cpp:141-158 J3DFrameCtrl::update, EMode_NONE (attribute 0, the B/C attack clips).
inline void ref_frame_update(float& mFrame,float& mRate,float mStart,float mEnd){
    mFrame += mRate;
    if (mFrame < mStart) { mFrame = mStart; mRate = 0.0f; }
    if (mFrame >= mEnd) { mFrame = mEnd - 0.001f; mRate = 0.0f; }
}
