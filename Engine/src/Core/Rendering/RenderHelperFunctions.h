#pragma once

#include <d3d11.h>
#include <d3dcompiler.h>

#include "Camera.h"
#include "../Structures/TransformCB.h"

ID3DBlob* CompileShader(const wchar_t* file, const char* entry, const char* model)
{
    ID3DBlob* shaderBlob = nullptr;
    ID3DBlob* errorBlob = nullptr;

    HRESULT hr = D3DCompileFromFile(
        file, nullptr, nullptr, entry, model,
        0, 0, &shaderBlob, &errorBlob
    );

    if (FAILED(hr))
    {
        if (errorBlob)
            errorBlob->Release();
        return nullptr;
    }

    return shaderBlob;
}

void MakeTransform(float x, float y, float scale, TransformCB& out)
{
    float m[16] =
    {
        scale, 0,     0, 0,
        0,     scale, 0, 0,
        0,     0,     1, 0,
        x,     y,     0, 1
    };

    memcpy(out.m, m, sizeof(m));
}

void MakeWorldMatrix(float x, float y, float scale, float out[16])
{
    float m[16] =
    {
        scale, 0,     0, 0,
        0,     scale, 0, 0,
        0,     0,     1, 0,
        x,     y,     0, 1
    };
    memcpy(out, m, sizeof(m));
}

void MakeViewMatrix(const Camera2D& cam, float out[16])
{
    float m[16] =
    {
        1, 0, 0, 0,
        0, 1, 0, 0,
        0, 0, 1, 0,
        -cam.GetX(), -cam.GetY(), 0, 1
    };
    memcpy(out, m, sizeof(m));
}

void MakeOrthoMatrix(float width, float height, float out[16])
{
    float m[16] =
    {
        2.0f / width, 0,               0, 0,
        0,            2.0f / height,   0, 0,
        0,            0,               1, 0,
        0,            0,               0, 1
    };
    memcpy(out, m, sizeof(m));
}

void Multiply(const float a[16], const float b[16], float out[16])
{
    float r[16];

    for (int row = 0; row < 4; ++row)
    {
        for (int col = 0; col < 4; ++col)
        {
            r[row * 4 + col] =
                a[row * 4 + 0] * b[0 * 4 + col] +
                a[row * 4 + 1] * b[1 * 4 + col] +
                a[row * 4 + 2] * b[2 * 4 + col] +
                a[row * 4 + 3] * b[3 * 4 + col];
        }
    }

    memcpy(out, r, sizeof(r));
}