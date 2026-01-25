
#include "Renderer.h"
#include "RenderHelperFunctions.h"
#include "..\Structures\TransformCB.h"
#include "..\Structures\Vertex.h"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <d3dcompiler.h>
#include <d3d11.h>
#include <dxgi.h>
#include <windows.h>

bool DX11Renderer::Init(HWND hwnd)
{
    m_hwnd = hwnd;

    // 1. Create device, context, swap chain
    DXGI_SWAP_CHAIN_DESC sd = {};
    sd.BufferCount = 1;
    sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    sd.OutputWindow = hwnd;
    sd.SampleDesc.Count = 1;
    sd.Windowed = TRUE;

    UINT createFlags = 0;

    D3D_FEATURE_LEVEL featureLevel;
    HRESULT hr = D3D11CreateDeviceAndSwapChain(
        nullptr,
        D3D_DRIVER_TYPE_HARDWARE,
        nullptr,
        createFlags,
        nullptr,
        0,
        D3D11_SDK_VERSION,
        &sd,
        &m_swapChain,
        &m_device,
        &featureLevel,
        &m_context
    );

    if (FAILED(hr))
        return false;

    // 2. Get backbuffer
    ID3D11Texture2D* backBuffer = nullptr;
    hr = m_swapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)&backBuffer);
    if (FAILED(hr))
        return false;

    // 3. Create RTV
    hr = m_device->CreateRenderTargetView(backBuffer, nullptr, &m_rtv);
    backBuffer->Release();
    if (FAILED(hr))
        return false;

    // 4. Bind RTV
    m_context->OMSetRenderTargets(1, &m_rtv, nullptr);

    // 5. NOW set viewport (correct placement)
    RECT rc;
    GetClientRect(hwnd, &rc);

    m_camera.viewport.TopLeftX = 0;
    m_camera.viewport.TopLeftY = 0;
    m_camera.viewport.Width = float(rc.right - rc.left);
    m_camera.viewport.Height = float(rc.bottom - rc.top);
    m_camera.viewport.MinDepth = 0.0f;
    m_camera.viewport.MaxDepth = 1.0f;

    m_context->RSSetViewports(1, &m_camera.viewport);

    // 6. Load/compile shaders (your existing code)
    D3D11_INPUT_ELEMENT_DESC layout[] =
    {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT,    0, 0,                      D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "COLOR",    0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, sizeof(float) * 3,     D3D11_INPUT_PER_VERTEX_DATA, 0 },
    };

    LoadCSOFile(L"VS.cso", L"PS.cso", *layout, &m_vertexShader, &m_pixelShader, &m_inputLayout);
    
    D3D11_INPUT_ELEMENT_DESC lineLayout[] =
    {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0,
          D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "COLOR",    0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12,
          D3D11_INPUT_PER_VERTEX_DATA, 0 },
    };
	LoadCSOFile(L"LineVS.cso", L"LinePS.cso", *lineLayout, &m_lineVS, &m_linePS, &m_lineLayout);

    // 7. Create input layout, buffers, etc.
    CreateGeometry();
    CreateConstantBuffers();

    return true;
}

bool DX11Renderer::LoadCSOFile(const LPCWSTR filenameVS, const LPCWSTR filenamePS, D3D11_INPUT_ELEMENT_DESC& inLayout, ID3D11VertexShader** outVS, ID3D11PixelShader** outPS, ID3D11InputLayout** outLayout)
{
    ID3DBlob* vsBlob = nullptr;
    ID3DBlob* psBlob = nullptr;

    D3DReadFileToBlob(filenameVS, &vsBlob);
    D3DReadFileToBlob(filenamePS, &psBlob);

    m_device->CreateVertexShader(
        vsBlob->GetBufferPointer(),
        vsBlob->GetBufferSize(),
        nullptr,
        outVS
    );

    m_device->CreatePixelShader(
        psBlob->GetBufferPointer(),
        psBlob->GetBufferSize(),
        nullptr,
        outPS
    );

    D3D11_INPUT_ELEMENT_DESC layout[] =
    {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0,
          D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "COLOR",    0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12,
          D3D11_INPUT_PER_VERTEX_DATA, 0 },
    };

    m_device->CreateInputLayout(
        &inLayout,
        ARRAYSIZE(layout),
        vsBlob->GetBufferPointer(),
        vsBlob->GetBufferSize(),
        outLayout
    );

    vsBlob->Release();
    psBlob->Release();

    return true;
}

void DX11Renderer::CreateConstantBuffers()
{
    D3D11_BUFFER_DESC cbd = {};
    cbd.Usage = D3D11_USAGE_DEFAULT;
    cbd.ByteWidth = sizeof(float) * 16; // 4x4 matrix
    cbd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;

    m_device->CreateBuffer(&cbd, nullptr, &m_transformCB);
}

void DX11Renderer::CreateGeometry()
{
    Vertex verts[] =
    {
        { -0.5f, -0.5f, 0.0f,   1, 0, 0, 1 },
        { -0.5f,  0.5f, 0.0f,   0, 1, 0, 1 },
        {  0.5f,  0.5f, 0.0f,   0, 0, 1, 1 },
        {  0.5f, -0.5f, 0.0f,   1, 1, 0, 1 },
    };

    uint32_t indices[] = { 0, 1, 2, 0, 2, 3 };

    // VB
    D3D11_BUFFER_DESC vbDesc = {};
    vbDesc.Usage = D3D11_USAGE_DEFAULT;
    vbDesc.ByteWidth = sizeof(verts);
    vbDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;

    D3D11_SUBRESOURCE_DATA vbData = {};
    vbData.pSysMem = verts;

    m_device->CreateBuffer(&vbDesc, &vbData, &m_vertexBuffer);

    // IB
    D3D11_BUFFER_DESC ibDesc = {};
    ibDesc.Usage = D3D11_USAGE_DEFAULT;
    ibDesc.ByteWidth = sizeof(indices);
    ibDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;

    D3D11_SUBRESOURCE_DATA ibData = {};
    ibData.pSysMem = indices;

    m_device->CreateBuffer(&ibDesc, &ibData, &m_indexBuffer);
}

void DX11Renderer::DrawQuad(float x, float y, float scale)
{
    float world[16];
    float view[16];
    float proj[16];
    float temp[16];
    float mvp[16];

    MakeWorldMatrix(x, y, scale, world);
    MakeViewMatrix(m_camera, view);
    MakeOrthoMatrix(m_camera.GetViewWidth() / m_camera.GetZoom(),
        m_camera.GetViewHeight() / m_camera.GetZoom(),
        proj);

    Multiply(world, view, temp);
    Multiply(temp, proj, mvp);

    TransformCB cb;
    memcpy(cb.m, mvp, sizeof(mvp));

    m_context->UpdateSubresource(m_transformCB, 0, nullptr, &cb, 0, 0);
    m_context->VSSetConstantBuffers(0, 1, &m_transformCB);

    m_context->Draw(6, 0);
}

void DX11Renderer::DrawLine(const XMFLOAT2& a, const XMFLOAT2& b, const XMFLOAT4& color)
{
    struct LineVertex
    {
        float x, y, z;
        float r, g, b, a;
    };

    LineVertex verts[2] =
    {
        { a.x, a.y, 0.0f, color.x, color.y, color.z, color.w },
        { b.x, b.y, 0.0f, color.x, color.y, color.z, color.w }
    };

    D3D11_BUFFER_DESC bd = {};
    bd.Usage = D3D11_USAGE_IMMUTABLE;
    bd.ByteWidth = sizeof(verts);
    bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;

    D3D11_SUBRESOURCE_DATA initData = {};
    initData.pSysMem = verts;

    ID3D11Buffer* vb = nullptr;
    m_device->CreateBuffer(&bd, &initData, &vb);

    UINT stride = sizeof(LineVertex);
    UINT offset = 0;

    m_context->IASetVertexBuffers(0, 1, &vb, &stride, &offset);
    m_context->IASetInputLayout(m_lineLayout);
    m_context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_LINELIST);

    m_context->VSSetShader(m_lineVS, nullptr, 0);
    m_context->PSSetShader(m_linePS, nullptr, 0);
    m_context->VSSetConstantBuffers(0, 1, &m_transformCB);

    m_context->Draw(2, 0);

    vb->Release();
}


void DX11Renderer::DrawGrid()
{
    float worldWidth = m_camera.GetViewWidth() / m_camera.GetZoom();
    float worldHeight = m_camera.GetViewHeight() / m_camera.GetZoom();

    float left = m_camera.GetX() - worldWidth * 0.5f;
    float right = m_camera.GetX() + worldWidth * 0.5f;
    float bottom = m_camera.GetY() - worldHeight * 0.5f;
    float top = m_camera.GetY() + worldHeight * 0.5f;

    int startX = (int)std::floor(left);
    int endX = (int)std::ceil(right);
    int startY = (int)std::floor(bottom);
    int endY = (int)std::ceil(top);

    XMFLOAT4 minorColor = { 0.25f, 0.25f, 0.25f, 1.0f };
    XMFLOAT4 majorColor = { 0.45f, 0.45f, 0.45f, 1.0f };

    for (int x = startX; x <= endX; x++)
    {
        const XMFLOAT4& c = (x % 10 == 0) ? majorColor : minorColor;
        DrawLine({ (float)x, bottom }, { (float)x, top }, c);
    }

    for (int y = startY; y <= endY; y++)
    {
        const XMFLOAT4& c = (y % 10 == 0) ? majorColor : minorColor;
        DrawLine({ left, (float)y }, { right, (float)y }, c);
    }
}

void DX11Renderer::RenderFrame()
{
    //
    // --- Clear screen ---
    //
    const float clearColor[4] = { 0.1f, 0.1f, 0.3f, 1.0f };
    m_context->ClearRenderTargetView(m_rtv, clearColor);

    //
    // --- Ensure viewport is active ---
    //
    m_context->RSSetViewports(1, &m_camera.viewport);

    //
    // --- Build MVP matrix from camera ---
    //
    float halfW = (m_camera.GetViewWidth() * 0.5f) / m_camera.GetZoom();
    float halfH = (m_camera.GetViewHeight() * 0.5f) / m_camera.GetZoom();

    float left = m_camera.GetX() - halfW;
    float right = m_camera.GetX() + halfW;
    float bottom = m_camera.GetY() - halfH;
    float top = m_camera.GetY() + halfH;

    XMMATRIX proj = XMMatrixOrthographicOffCenterLH(left, right, bottom, top, 0.0f, 1.0f);
    XMMATRIX view = XMMatrixIdentity();
    XMMATRIX model = XMMatrixIdentity();
    XMMATRIX mvp = model * view * proj;

    //
    // --- Upload MVP to constant buffer ---
    //
    m_context->UpdateSubresource(m_transformCB, 0, nullptr, &mvp, 0, 0);

    //
    // --- Draw grid (uses line pipeline inside DrawLine) ---
    //
    DrawGrid();

    //
    // --- Draw axis lines ---
    //
    {
        XMFLOAT4 xColor = { 1.0f, 0.2f, 0.2f, 1.0f };
        XMFLOAT4 yColor = { 0.2f, 1.0f, 0.2f, 1.0f };

        if (0 >= bottom && 0 <= top)
            DrawLine({ left, 0.0f }, { right, 0.0f }, xColor);

        if (0 >= left && 0 <= right)
            DrawLine({ 0.0f, bottom }, { 0.0f, top }, yColor);
    }

    //
    // --- Switch to quad pipeline ---
    //
    UINT stride = sizeof(Vertex);
    UINT offset = 0;

    m_context->IASetInputLayout(m_inputLayout);
    m_context->IASetVertexBuffers(0, 1, &m_vertexBuffer, &stride, &offset);
    m_context->IASetIndexBuffer(m_indexBuffer, DXGI_FORMAT_R32_UINT, 0);
    m_context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    m_context->VSSetShader(m_vertexShader, nullptr, 0);
    m_context->PSSetShader(m_pixelShader, nullptr, 0);
    m_context->VSSetConstantBuffers(0, 1, &m_transformCB);

    //
    // --- Draw quad ---
    //
    m_context->DrawIndexed(6, 0, 0);

    //
    // --- Present ---
    //
    m_swapChain->Present(1, 0);
}

void DX11Renderer::Shutdown()
{
	m_hwnd = nullptr;
	m_camera = {};

    if (m_rtv)        m_rtv->Release();
    if (m_swapChain)  m_swapChain->Release();
    if (m_context)    m_context->Release();
    if (m_device)     m_device->Release();
	if (m_vertexBuffer)   m_vertexBuffer->Release();
	if (m_indexBuffer)    m_indexBuffer->Release();
	if (m_inputLayout)    m_inputLayout->Release();
	if (m_vertexShader)   m_vertexShader->Release();
    if (m_pixelShader)    m_pixelShader->Release();
	if (m_transformCB)   m_transformCB->Release();
}
