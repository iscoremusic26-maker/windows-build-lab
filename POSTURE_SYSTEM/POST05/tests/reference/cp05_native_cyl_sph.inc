bool cM3d_Cross_CylSph(cM3dGCyl const* cyl, cM3dGSph const* sph, f32* param_2) {
    const cXyz* sph_center = sph->GetCP();
    const cXyz* cyl_center = cyl->GetCP();
    f32 radius_sum = cyl->GetR() + sph->GetR();
    f32 dist = sqrtf(cM3d_Len2dSq(sph_center->x, sph_center->z, cyl_center->x, cyl_center->z));

    if (radius_sum < dist) {
        return false;
    }

    if (sph_center->y + sph->GetR() >= cyl_center->y
        && sph_center->y - sph->GetR() <= cyl_center->y + cyl->GetH())
    {
        *param_2 = radius_sum - dist;
        return true;
    }

    return false;
}
