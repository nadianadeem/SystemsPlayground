
#include "Renderer.h"
#include "..\Structures\TransformCB.h"
#include "..\Structures\Vertex.h"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <d3dcompiler.h>
#include <d3d11.h>
#include <dxgi.h>
#include <windows.h>

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
        -cam.x, -cam.y, 0, 1
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

    m_viewport.TopLeftX = 0;
    m_viewport.TopLeftY = 0;
    m_viewport.Width = float(rc.right - rc.left);
    m_viewport.Height = float(rc.bottom - rc.top);
    m_viewport.MinDepth = 0.0f;
    m_viewport.MaxDepth = 1.0f;

    m_context->RSSetViewports(1, &m_viewport);

    // 6. Load/compile shaders (your existing code)
    LoadShaders();

    // 7. Create input layout, buffers, etc.
    CreateGeometry();
    CreateConstantBuffers();

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


void DX11Renderer::LoadShaders() 
{
    //Done so we don't compile at runtime.
    ID3DBlob* vsBlob = nullptr;
    D3DReadFileToBlob(L"VS.cso", &vsBlob);

    ID3DBlob* psBlob = nullptr;
    D3DReadFileToBlob(L"PS.cso", &psBlob);

    m_device->CreateVertexShader(
        vsBlob->GetBufferPointer(),
        vsBlob->GetBufferSize(),
        nullptr,
        &m_vertexShader
    );

    m_device->CreatePixelShader(
        psBlob->GetBufferPointer(),
        psBlob->GetBufferSize(),
        nullptr,
        &m_pixelShader
    );

    // after creating m_vertexShader:
    D3D11_INPUT_ELEMENT_DESC layout[] =
    {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT,    0, 0,                      D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "COLOR",    0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, sizeof(float) * 3,     D3D11_INPUT_PER_VERTEX_DATA, 0 },
    };


    m_device->CreateInputLayout(
        layout,
        ARRAYSIZE(layout),
        vsBlob->GetBufferPointer(),
        vsBlob->GetBufferSize(),
        &m_inputLayout
    );

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
    MakeOrthoMatrix(m_camera.viewWidth / m_camera.zoom,
        m_camera.viewHeight / m_camera.zoom,
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
    bd.Usage = D3D11_USAGE_DYNAMIC;
    bd.ByteWidth = sizeof(verts);
    bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    bd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

    D3D11_SUBRESOURCE_DATA initData = {};
    initData.pSysMem = verts;

    ID3D11Buffer* vb = nullptr;
    m_device->CreateBuffer(&bd, &initData, &vb);

    // Bind the line vertex buffer
    UINT stride = sizeof(LineVertex);
    UINT offset = 0;
    m_context->IASetVertexBuffers(0, 1, &vb, &stride, &offset);

    // Re-bind the input layout (this is the missing piece)
    m_context->IASetInputLayout(m_inputLayout);

    // Re-bind the topology (quad uses TRIANGLELIST)
    m_context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_LINELIST);

    // Shaders + constant buffer
    m_context->VSSetShader(m_vertexShader, nullptr, 0);
    m_context->PSSetShader(m_pixelShader, nullptr, 0);
    m_context->VSSetConstantBuffers(0, 1, &m_transformCB);

    m_context->Draw(2, 0);

    vb->Release();
}

void DX11Renderer::DrawGrid()
{
    float worldWidth = m_camera.viewWidth / m_camera.zoom;
    float worldHeight = m_camera.viewHeight / m_camera.zoom;

    float left = m_camera.x - worldWidth * 0.5f;
    float right = m_camera.x + worldWidth * 0.5f;
    float bottom = m_camera.y - worldHeight * 0.5f;
    float top = m_camera.y + worldHeight * 0.5f;

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
    // --- Clear screen ---
    float clearColor[4] = { 0.1f, 0.1f, 0.3f, 1.0f };
    m_context->ClearRenderTargetView(m_rtv, clearColor);

    // --- Ensure viewport is set every frame ---
    m_context->RSSetViewports(1, &m_viewport);

	// --- Build MVP matrix based on camera ---
    float halfW = (m_camera.viewWidth * 0.5f) / m_camera.zoom;
    float halfH = (m_camera.viewHeight * 0.5f) / m_camera.zoom;

    float left = m_camera.x - halfW;
    float right = m_camera.x + halfW;
    float bottom = m_camera.y - halfH;
    float top = m_camera.y + halfH;

    XMMATRIX proj = XMMatrixOrthographicOffCenterLH(left, right, bottom, top, 0.0f, 1.0f);
    XMMATRIX view = XMMatrixIdentity();
    XMMATRIX model = XMMatrixIdentity();

    XMMATRIX mvp = model * view * proj;

    // Upload to constant buffer
    m_context->UpdateSubresource(m_transformCB, 0, nullptr, &mvp, 0, 0);

    // --- Bind pipeline state ---
    m_context->IASetInputLayout(m_inputLayout);
    m_context->VSSetShader(m_vertexShader, nullptr, 0);
    m_context->PSSetShader(m_pixelShader, nullptr, 0);

    // --- Bind geometry ---
    UINT stride = sizeof(float) * 7; // 3 pos + 4 color
    UINT offset = 0;

    m_context->IASetVertexBuffers(0, 1, &m_vertexBuffer, &stride, &offset);
    m_context->IASetIndexBuffer(m_indexBuffer, DXGI_FORMAT_R32_UINT, 0);
    m_context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    // --- Bind constant buffer ---
    m_context->VSSetConstantBuffers(0, 1, &m_transformCB);

    // --- Draw ---
    DrawGrid();

    // --- Draw Axis Lines ---
    {
        XMFLOAT4 xColor = { 1.0f, 0.2f, 0.2f, 1.0f }; // red
        XMFLOAT4 yColor = { 0.2f, 1.0f, 0.2f, 1.0f }; // green

        // X-axis (horizontal line at y = 0)
        if (0 >= bottom && 0 <= top)
            DrawLine({ left, 0.0f }, { right, 0.0f }, xColor);

        // Y-axis (vertical line at x = 0)
        if (0 >= left && 0 <= right)
            DrawLine({ 0.0f, bottom }, { 0.0f, top }, yColor);
    }

    // --- Present ---
    m_swapChain->Present(1, 0);
}


XMFLOAT2 DX11Renderer::ScreenToWorld(int sx, int sy)
{
    float ndcX = (2.0f * sx / m_viewport.Width) - 1.0f;
    float ndcY = 1.0f - (2.0f * sy / m_viewport.Height);

    float worldWidth = m_camera.viewWidth / m_camera.zoom;
    float worldHeight = m_camera.viewHeight / m_camera.zoom;

    float worldX = m_camera.x + ndcX * (worldWidth * 0.5f);
    float worldY = m_camera.y + ndcY * (worldHeight * 0.5f);

    return { worldX, worldY };
}

void DX11Renderer::UpdateCamera(float dt)
{
    const float moveSpeed = 10.0f;   // world units per second
    const float zoomSpeed = 1.5f;    // zoom multiplier per second

    // Pan
    if (GetAsyncKeyState(VK_LEFT) & 0x8000)
        m_camera.x -= moveSpeed * dt;

    if (GetAsyncKeyState(VK_RIGHT) & 0x8000)
        m_camera.x += moveSpeed * dt;

    if (GetAsyncKeyState(VK_UP) & 0x8000)
        m_camera.y += moveSpeed * dt;

    if (GetAsyncKeyState(VK_DOWN) & 0x8000)
        m_camera.y -= moveSpeed * dt;

    // Zoom in/out
    if (GetAsyncKeyState('Q') & 0x8000)
        m_camera.zoom *= (1.0f + zoomSpeed * dt);

    if (GetAsyncKeyState('E') & 0x8000)
        m_camera.zoom *= (1.0f - zoomSpeed * dt);

    // Clamp zoom so it never flips or goes negative
    if (m_camera.zoom < 0.1f)
        m_camera.zoom = 0.1f;

    if (!std::isfinite(m_camera.x)) m_camera.x = 0.0f;
    if (!std::isfinite(m_camera.y)) m_camera.y = 0.0f;
    if (!std::isfinite(m_camera.zoom)) m_camera.zoom = 1.0f;
}

void DX11Renderer::OnMouseMove(int x, int y)
{
    if (!m_middleDown)
    {
        m_lastMousePos.x = x;
        m_lastMousePos.y = y;
        return;
    }

    int dx = x - m_lastMousePos.x;
    int dy = y - m_lastMousePos.y;

    m_lastMousePos.x = x;
    m_lastMousePos.y = y;

    // Convert pixel delta to world delta
    float worldWidth = m_camera.viewWidth / m_camera.zoom;
    float worldHeight = m_camera.viewHeight / m_camera.zoom;

    float worldPerPixelX = worldWidth / m_viewport.Width;
    float worldPerPixelY = worldHeight / m_viewport.Height;

    // Move camera opposite to mouse drag
    m_camera.x -= dx * worldPerPixelX;
    m_camera.y += dy * worldPerPixelY; // + because screen Y is inverted
}

void DX11Renderer::OnMouseWheel(short delta)
{
    // Get cursor in client space
    POINT p;
    GetCursorPos(&p);
    ScreenToClient(m_hwnd, &p);

    // World position before zoom
    XMFLOAT2 before = ScreenToWorld(p.x, p.y);

    // Logarithmic zoom
    float zoomFactor = std::exp((delta / 120.0f) * 0.1f);
    m_camera.zoom *= zoomFactor;

    // Clamp zoom to safe range
    m_camera.zoom = std::clamp(m_camera.zoom, 0.5f, 4.0f);

    // World position after zoom
    XMFLOAT2 after = ScreenToWorld(p.x, p.y);

    // Shift camera so the point stays under the cursor
    m_camera.x += (before.x - after.x);
    m_camera.y += (before.y - after.y);
}

void DX11Renderer::Shutdown()
{
	m_hwnd = nullptr;
	m_viewport = {};
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
