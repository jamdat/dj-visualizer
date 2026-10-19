#pragma once

#include <cmath>
#include <algorithm>

namespace Visualizer::Graphics {

struct Mat4 {
    float m[16] = {0}; // column-major for OpenGL glUniformMatrix4fv

    static Mat4 identity() {
        Mat4 r;
        r.m[0] = 1.0f; r.m[5] = 1.0f; r.m[10] = 1.0f; r.m[15] = 1.0f;
        return r;
    }

    static Mat4 translation(float tx, float ty, float tz) {
        Mat4 r = identity();
        r.m[12] = tx;
        r.m[13] = ty;
        r.m[14] = tz;
        return r;
    }

    static Mat4 scale(float sx, float sy, float sz) {
        Mat4 r;
        r.m[0] = sx;
        r.m[5] = sy;
        r.m[10] = sz;
        r.m[15] = 1.0f;
        return r;
    }

    static Mat4 rotationY(float rad) {
        Mat4 r = identity();
        float c = std::cos(rad);
        float s = std::sin(rad);
        r.m[0] = c;  r.m[8] = s;
        r.m[2] = -s; r.m[10] = c;
        return r;
    }

    static Mat4 rotationX(float rad) {
        Mat4 r = identity();
        float c = std::cos(rad);
        float s = std::sin(rad);
        r.m[5] = c;  r.m[9] = -s;
        r.m[6] = s;  r.m[10] = c;
        return r;
    }

    static Mat4 perspective(float fovRad, float aspect, float zNear, float zFar) {
        Mat4 r;
        float tanHalfFov = std::tan(fovRad * 0.5f);
        r.m[0] = 1.0f / (aspect * tanHalfFov);
        r.m[5] = 1.0f / tanHalfFov;
        r.m[10] = -(zFar + zNear) / (zFar - zNear);
        r.m[11] = -1.0f;
        r.m[14] = -(2.0f * zFar * zNear) / (zFar - zNear);
        return r;
    }

    static Mat4 lookAt(float eyeX, float eyeY, float eyeZ,
                       float targetX, float targetY, float targetZ,
                       float upX, float upY, float upZ) {
        float fX = targetX - eyeX;
        float fY = targetY - eyeY;
        float fZ = targetZ - eyeZ;
        float lenF = std::sqrt(fX * fX + fY * fY + fZ * fZ);
        if (lenF < 1e-6f) lenF = 1.0f;
        fX /= lenF; fY /= lenF; fZ /= lenF;

        // s = f x up
        float sX = fY * upZ - fZ * upY;
        float sY = fZ * upX - fX * upZ;
        float sZ = fX * upY - fY * upX;
        float lenS = std::sqrt(sX * sX + sY * sY + sZ * sZ);
        if (lenS < 1e-6f) lenS = 1.0f;
        sX /= lenS; sY /= lenS; sZ /= lenS;

        // u = s x f
        float uX = sY * fZ - sZ * fY;
        float uY = sZ * fX - sX * fZ;
        float uZ = sX * fY - sY * fX;

        Mat4 r;
        r.m[0] = sX;  r.m[4] = sY;  r.m[8]  = sZ;  r.m[12] = -(sX * eyeX + sY * eyeY + sZ * eyeZ);
        r.m[1] = uX;  r.m[5] = uY;  r.m[9]  = uZ;  r.m[13] = -(uX * eyeX + uY * eyeY + uZ * eyeZ);
        r.m[2] = -fX; r.m[6] = -fY; r.m[10] = -fZ; r.m[14] =  (fX * eyeX + fY * eyeY + fZ * eyeZ);
        r.m[3] = 0.0f; r.m[7] = 0.0f; r.m[11] = 0.0f; r.m[15] = 1.0f;
        return r;
    }

    Mat4 operator*(const Mat4& b) const {
        Mat4 r;
        for (int row = 0; row < 4; ++row) {
            for (int col = 0; col < 4; ++col) {
                float sum = 0.0f;
                for (int k = 0; k < 4; ++k) {
                    sum += m[k * 4 + row] * b.m[col * 4 + k];
                }
                r.m[col * 4 + row] = sum;
            }
        }
        return r;
    }
};

} // namespace Visualizer::Graphics
