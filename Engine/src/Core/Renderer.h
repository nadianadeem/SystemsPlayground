#pragma once

#include <d3d11.h>

class DX11Renderer
{
public:
    bool Init(HWND hwnd);
    void RenderFrame();
    void Shutdown();

private:
    ID3D11Device* m_device = nullptr;
    ID3D11DeviceContext* m_context = nullptr;
    IDXGISwapChain* m_swapChain = nullptr;
    ID3D11RenderTargetView* m_rtv = nullptr;
};
