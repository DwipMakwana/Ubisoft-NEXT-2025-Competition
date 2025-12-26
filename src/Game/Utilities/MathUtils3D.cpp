//-----------------------------------------------------------------------------
// MathUtils3D.cpp
// Implementation of 3D math
//-----------------------------------------------------------------------------
#include "MathUtils3D.h"
#include <cmath>

#ifndef PI
#define PI 3.14159265359f
#endif

Mat4 Mat4::Translation(float x, float y, float z) {
    Mat4 result;
    result.m[12] = x;
    result.m[13] = y;
    result.m[14] = z;
    return result;
}

Mat4 Mat4::Scale(float x, float y, float z) {
    Mat4 result;
    result.m[0] = x;
    result.m[5] = y;
    result.m[10] = z;
    return result;
}

Mat4 Mat4::RotationX(float angle) {
    Mat4 result;
    float c = cosf(angle);
    float s = sinf(angle);
    result.m[5] = c;
    result.m[6] = s;
    result.m[9] = -s;
    result.m[10] = c;
    return result;
}

Mat4 Mat4::RotationY(float angle) {
    Mat4 result;
    float c = cosf(angle);
    float s = sinf(angle);
    result.m[0] = c;
    result.m[2] = -s;
    result.m[8] = s;
    result.m[10] = c;
    return result;
}

Mat4 Mat4::RotationZ(float angle) {
    Mat4 result;
    float c = cosf(angle);
    float s = sinf(angle);
    result.m[0] = c;
    result.m[1] = s;
    result.m[4] = -s;
    result.m[5] = c;
    return result;
}

Mat4 Mat4::operator*(const Mat4& other) const {
    Mat4 result;
    for (int row = 0; row < 4; row++) {
        for (int col = 0; col < 4; col++) {
            result.m[col * 4 + row] = 0;
            for (int k = 0; k < 4; k++) {
                result.m[col * 4 + row] += m[k * 4 + row] * other.m[col * 4 + k];
            }
        }
    }
    return result;
}

Vec4 Mat4::operator*(const Vec4& v) const {
    return Vec4(
        m[0] * v.x + m[4] * v.y + m[8] * v.z + m[12] * v.w,
        m[1] * v.x + m[5] * v.y + m[9] * v.z + m[13] * v.w,
        m[2] * v.x + m[6] * v.y + m[10] * v.z + m[14] * v.w,
        m[3] * v.x + m[7] * v.y + m[11] * v.z + m[15] * v.w
    );
}
