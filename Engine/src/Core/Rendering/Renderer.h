#pragma once

#include <d3d11.h>
#include <DirectXMath.h>

using namespace DirectX;

struct Camera2D
{
    float x = 0.0f;
    float y = 0.0f;
    float zoom = 1.0f;

    // World units visible at zoom = 1
    float viewWidth = 30.0f;
    float viewHeight = 30.0f;
};


class DX11Renderer
{
public:
    bool Init(HWND hwnd);
    void LoadShaders();
    void CreateConstantBuffers();
    void CreateGeometry();
    void DrawQuad(float x, float y, float scale);
    void RenderFrame();
    XMFLOAT2 ScreenToWorld(int sx, int sy);
    void UpdateCamera(float dt);
    void OnMouseMove(int x, int y);
    void OnMouseWheel(short delta);
    void Shutdown();

    POINT m_lastMousePos = { 0, 0 };
    bool m_middleDown = false;

private:
    ID3D11Device* m_device = nullptr;
    ID3D11DeviceContext* m_context = nullptr;
    IDXGISwapChain* m_swapChain = nullptr;
    ID3D11RenderTargetView* m_rtv = nullptr;
    HWND m_hwnd = nullptr;

    ID3D11Buffer* m_vertexBuffer = nullptr;
    ID3D11Buffer* m_indexBuffer = nullptr;
    ID3D11InputLayout* m_inputLayout = nullptr;
    ID3D11VertexShader* m_vertexShader = nullptr;
    ID3D11PixelShader* m_pixelShader = nullptr;

    D3D11_VIEWPORT m_viewport;
    Camera2D m_camera;
    ID3D11Buffer* m_transformCB = nullptr;
};
