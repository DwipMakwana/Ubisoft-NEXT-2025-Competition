//-----------------------------------------------------------------------------
// MathUtils3D.h
// 3D math utilities for 3D game development
//-----------------------------------------------------------------------------
#ifndef MATH_UTILS_3D_H
#define MATH_UTILS_3D_H

#include <cmath>

//-----------------------------------------------------------------------------
// 2D Vector
//-----------------------------------------------------------------------------
struct Vec2 {
    float x, y;

    Vec2() : x(0), y(0) {}
    Vec2(float _x, float _y) : x(_x), y(_y) {}

    Vec2 operator+(const Vec2& other) const { return Vec2(x + other.x, y + other.y); }
    Vec2 operator-(const Vec2& other) const { return Vec2(x - other.x, y - other.y); }
    Vec2 operator*(float scalar) const { return Vec2(x * scalar, y * scalar); }
    Vec2& operator+=(const Vec2& other) { x += other.x; y += other.y; return *this; }

    float Length() const { return sqrtf(x * x + y * y); }
    float LengthSquared() const { return x * x + y * y; }

    Vec2 Normalized() const {
        float len = Length();
        return len > 0 ? Vec2(x / len, y / len) : Vec2(0, 0);
    }

    float Distance(const Vec2& other) const {
        return (*this - other).Length();
    }

    float Dot(const Vec2& other) const {
        return x * other.x + y * other.y;
    }
};

//-----------------------------------------------------------------------------
// 3D Vector
//-----------------------------------------------------------------------------
struct Vec3 {
    float x, y, z;

    Vec3() : x(0), y(0), z(0) {}
    Vec3(float _x, float _y, float _z) : x(_x), y(_y), z(_z) {}

    // Operators
    Vec3 operator+(const Vec3& other) const { return Vec3(x + other.x, y + other.y, z + other.z); }
    Vec3 operator-(const Vec3& other) const { return Vec3(x - other.x, y - other.y, z - other.z); }
    Vec3 operator*(float scalar) const { return Vec3(x * scalar, y * scalar, z * scalar); }
    Vec3& operator+=(const Vec3& other) { x += other.x; y += other.y; z += other.z; return *this; }

    // Vector operations
    float Length() const { return sqrtf(x * x + y * y + z * z); }
    float LengthSquared() const { return x * x + y * y + z * z; }

    Vec3 Normalized() const {
        float len = Length();
        return len > 0 ? Vec3(x / len, y / len, z / len) : Vec3(0, 0, 0);
    }

    float Distance(const Vec3& other) const {
        return (*this - other).Length();
    }

    float Dot(const Vec3& other) const {
        return x * other.x + y * other.y + z * other.z;
    }

    Vec3 Cross(const Vec3& other) const {
        return Vec3(
            y * other.z - z * other.y,
            z * other.x - x * other.z,
            x * other.y - y * other.x
        );
    }

    Vec3 operator/(float scalar) const {
        return Vec3(x / scalar, y / scalar, z / scalar);
    }

    Vec3& operator/=(float scalar) {
        x /= scalar;
        y /= scalar;
        z /= scalar;
        return *this;
    }
};

//-----------------------------------------------------------------------------
// 4D Vector (for homogeneous coordinates)
//-----------------------------------------------------------------------------
struct Vec4 {
    float x, y, z, w;

    Vec4() : x(0), y(0), z(0), w(1) {}
    Vec4(float _x, float _y, float _z, float _w = 1.0f) : x(_x), y(_y), z(_z), w(_w) {}
    Vec4(const Vec3& v3, float _w = 1.0f) : x(v3.x), y(v3.y), z(v3.z), w(_w) {}

    Vec3 ToVec3() const { return Vec3(x, y, z); }
};

//-----------------------------------------------------------------------------
// 4x4 Matrix (for transformations)
//-----------------------------------------------------------------------------
struct Mat4 {
    float m[16]; // Column-major order (OpenGL style)

    Mat4() {
        Identity();
    }

    void Identity() {
        for (int i = 0; i < 16; i++) m[i] = 0;
        m[0] = m[5] = m[10] = m[15] = 1.0f;
    }

    static Mat4 Translation(float x, float y, float z);
    static Mat4 Scale(float x, float y, float z);
    static Mat4 RotationX(float angle);
    static Mat4 RotationY(float angle);
    static Mat4 RotationZ(float angle);

    Mat4 operator*(const Mat4& other) const;
    Vec4 operator*(const Vec4& v) const;
};

//-----------------------------------------------------------------------------
// Matrix4x4 - 4x4 transformation matrix
//-----------------------------------------------------------------------------
struct Matrix4x4 {
    float m[16];  // Column-major order (OpenGL style)

    Matrix4x4() {
        // Identity matrix
        for (int i = 0; i < 16; i++) m[i] = 0.0f;
        m[0] = m[5] = m[10] = m[15] = 1.0f;
    }

    static Matrix4x4 Identity() {
        return Matrix4x4();
    }

    static Matrix4x4 Translation(const Vec3& t) {
        Matrix4x4 mat;
        mat.m[12] = t.x;
        mat.m[13] = t.y;
        mat.m[14] = t.z;
        return mat;
    }

    static Matrix4x4 Scale(const Vec3& s) {
        Matrix4x4 mat;
        mat.m[0] = s.x;
        mat.m[5] = s.y;
        mat.m[10] = s.z;
        return mat;
    }

    static Matrix4x4 RotationX(float angleRadians) {
        Matrix4x4 mat;
        float c = cosf(angleRadians);
        float s = sinf(angleRadians);
        mat.m[5] = c;
        mat.m[6] = s;
        mat.m[9] = -s;
        mat.m[10] = c;
        return mat;
    }

    static Matrix4x4 RotationY(float angleRadians) {
        Matrix4x4 mat;
        float c = cosf(angleRadians);
        float s = sinf(angleRadians);
        mat.m[0] = c;
        mat.m[2] = -s;
        mat.m[8] = s;
        mat.m[10] = c;
        return mat;
    }

    static Matrix4x4 RotationZ(float angleRadians) {
        Matrix4x4 mat;
        float c = cosf(angleRadians);
        float s = sinf(angleRadians);
        mat.m[0] = c;
        mat.m[1] = s;
        mat.m[4] = -s;
        mat.m[5] = c;
        return mat;
    }

    // Multiply two matrices
    Matrix4x4 operator*(const Matrix4x4& other) const {
        Matrix4x4 result;
        for (int row = 0; row < 4; row++) {
            for (int col = 0; col < 4; col++) {
                result.m[col * 4 + row] = 0.0f;
                for (int k = 0; k < 4; k++) {
                    result.m[col * 4 + row] += m[k * 4 + row] * other.m[col * 4 + k];
                }
            }
        }
        return result;
    }

    // Transform a Vec3 (treating as point with w=1)
    Vec3 TransformPoint(const Vec3& v) const {
        float x = m[0] * v.x + m[4] * v.y + m[8] * v.z + m[12];
        float y = m[1] * v.x + m[5] * v.y + m[9] * v.z + m[13];
        float z = m[2] * v.x + m[6] * v.y + m[10] * v.z + m[14];
        return Vec3(x, y, z);
    }

    // Transform a Vec3 (treating as direction with w=0)
    Vec3 TransformDirection(const Vec3& v) const {
        float x = m[0] * v.x + m[4] * v.y + m[8] * v.z;
        float y = m[1] * v.x + m[5] * v.y + m[9] * v.z;
        float z = m[2] * v.x + m[6] * v.y + m[10] * v.z;
        return Vec3(x, y, z);
    }
};

#endif // MATH_UTILS_3D_H
