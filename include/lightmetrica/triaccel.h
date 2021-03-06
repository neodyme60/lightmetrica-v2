/*
    Lightmetrica - A modern, research-oriented renderer

    Copyright (c) 2015 Hisanari Otsu

    Permission is hereby granted, free of charge, to any person obtaining a copy
    of this software and associated documentation files (the "Software"), to deal
    in the Software without restriction, including without limitation the rights
    to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
    copies of the Software, and to permit persons to whom the Software is
    furnished to do so, subject to the following conditions:

    The above copyright notice and this permission notice shall be included in
    all copies or substantial portions of the Software.

    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
    IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
    FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
    AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
    LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
    OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
    THE SOFTWARE.
*/

#pragma once

#include <lightmetrica/math.h>
#include <lightmetrica/ray.h>

LM_NAMESPACE_BEGIN

struct TriAccelTriangle
{

    uint32_t k;
    Float n_u;
    Float n_v;
    Float n_d;

    Float a_u;
    Float a_v;
    Float b_nu;
    Float b_nv;

    Float c_nu;
    Float c_nv;
    uint32_t faceIndex;
    uint32_t primIndex;

    auto Load(const Vec3& A, const Vec3& B, const Vec3& C) -> int
    {
        static const int waldModulo[4] = { 1, 2, 0, 1 };

        Vec3 b = C - A;
        Vec3 c = B - A;
        Vec3 N = Math::Cross(c, b);

        k = 0;
        // Determine the largest projection axis
        for (int j = 0; j < 3; j++)
        {
            if (Math::Abs(N[j]) > Math::Abs(N[k]))
            {
                k = j;
            }
        }

        uint32_t u = waldModulo[k];
        uint32_t v = waldModulo[k + 1];
        const Float n_k = N[k];
        const Float denom = b[u] * c[v] - b[v] * c[u];

        if (denom == 0)
        {
            k = 3;
            return 1;
        }

        // Pre-compute intersection calculation constants
        n_u = N[u] / n_k;
        n_v = N[v] / n_k;
        n_d = Math::Dot(Vec3(A), N) / n_k;
        b_nu = b[u] / denom;
        b_nv = -b[v] / denom;
        a_u = A[u];
        a_v = A[v];
        c_nu = c[v] / denom;
        c_nv = -c[u] / denom;

        return 0;
    }

    auto Intersect(const Ray& ray, Float mint, Float maxt, Float& u, Float& v, Float& t) const -> bool
    {
        Float o_u, o_v, o_k, d_u, d_v, d_k;
        switch (k)
        {
            case 0:
                o_u = ray.o[1];
                o_v = ray.o[2];
                o_k = ray.o[0];
                d_u = ray.d[1];
                d_v = ray.d[2];
                d_k = ray.d[0];
                break;

            case 1:
                o_u = ray.o[2];
                o_v = ray.o[0];
                o_k = ray.o[1];
                d_u = ray.d[2];
                d_v = ray.d[0];
                d_k = ray.d[1];
                break;

            case 2:
                o_u = ray.o[0];
                o_v = ray.o[1];
                o_k = ray.o[2];
                d_u = ray.d[0];
                d_v = ray.d[1];
                d_k = ray.d[2];
                break;

            default:
                return false;
        }

        const auto demon = d_u * n_u + d_v * n_v + d_k;
        if (demon == 0)
        {
            return false;
        }

        // Calculate the plane intersection
        t = (n_d - o_u*n_u - o_v*n_v - o_k) / demon;
        if (t < mint || t > maxt)
        {
            return false;
        }

        // Calculate the projected plane intersection point
        const Float hu = o_u + t * d_u - a_u;
        const Float hv = o_v + t * d_v - a_v;

        // In barycentric coordinates
        u = hv * b_nu + hu * b_nv;
        v = hu * c_nu + hv * c_nv;

        return u >= 0_f && v >= 0_f && u + v <= 1_f;
    }

};

LM_NAMESPACE_END
