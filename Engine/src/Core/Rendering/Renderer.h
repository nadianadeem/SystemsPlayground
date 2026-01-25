#pragma once

#include <d3d11.h>
#include <DirectXMath.h>

#include "Camera.h"

using namespace DirectX;

class DX11Renderer
{
public:
    bool Init(HWND hwnd);

    bool LoadCSOFile(const LPCWSTR filenameVS, const LPCWSTR filenamePS, D3D11_INPUT_ELEMENT_DESC& inLayout, ID3D11VertexShader** outVS, ID3D11PixelShader** outPS, ID3D11InputLayout** outLayout);

    void CreateConstantBuffers();
    void CreateGeometry();
    void DrawQuad(float x, float y, float scale);
    void DrawLine(const XMFLOAT2& a, const XMFLOAT2& b, const XMFLOAT4& color);
    void DrawGrid();
    void RenderFrame();
    void Shutdown();

	Camera2D& GetCamera() { return m_camera; }

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

    ID3D11VertexShader* m_lineVS = nullptr;
    ID3D11PixelShader* m_linePS = nullptr;
    ID3D11InputLayout* m_lineLayout = nullptr;

    Camera2D m_camera;
    ID3D11Buffer* m_transformCB = nullptr;
};
