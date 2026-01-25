
#include "Renderer.h"
#include "..\Structures\TransformCB.h"
#include "..\Structures\Vertex.h"

#include <windows.h>
#include <d3d11.h>
#include <dxgi.h>
#include <d3dcompiler.h>
#include <chrono>

#include <d3dcompiler.h>

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
    // Create the swap chain + device + context
    DXGI_SWAP_CHAIN_DESC scd = {};
    scd.BufferCount = 1;
    scd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    scd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    scd.OutputWindow = hwnd;
    scd.SampleDesc.Count = 1;
    scd.Windowed = TRUE;

	m_hwnd = hwnd;

    {
        HRESULT hr = D3D11CreateDeviceAndSwapChain(
            nullptr,
            D3D_DRIVER_TYPE_HARDWARE,
            nullptr,
            0,
            nullptr,
            0,
            D3D11_SDK_VERSION,
            &scd,
            &m_swapChain,
            &m_device,
            nullptr,
            &m_context
        );

        if (FAILED(hr))
            return false;
    }


	// Create Transform Constant Buffer
    D3D11_BUFFER_DESC cbd = {};
    cbd.Usage = D3D11_USAGE_DEFAULT;
    cbd.ByteWidth = sizeof(TransformCB);
    cbd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;

    HRESULT hr = m_device->CreateBuffer(&cbd, nullptr, &m_transformCB);
    if (FAILED(hr))
        return false;

    // Render Target View
    ID3D11Texture2D* backBuffer = nullptr;
    m_swapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)&backBuffer);

    hr = m_device->CreateRenderTargetView(backBuffer, nullptr, &m_rtv);
    backBuffer->Release();

    if (FAILED(hr))
        return false;

    // Compile shaders
    ID3DBlob* vsBlob = nullptr;
    D3DReadFileToBlob(L"VS.cso", &vsBlob);

    ID3DBlob* psBlob = nullptr;
    D3DReadFileToBlob(L"PS.cso", &psBlob);

    if (!vsBlob || !psBlob)
        return false;

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

    //
    // 4. Create input layout
    //
    D3D11_INPUT_ELEMENT_DESC layout[] =
    {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT,    0, 0,  D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "COLOR",    0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 },
    };

    m_device->CreateInputLayout(
        layout,
        2,
        vsBlob->GetBufferPointer(),
        vsBlob->GetBufferSize(),
        &m_inputLayout
    );

    vsBlob->Release();
    psBlob->Release();

    //
    // 5. Create a quad vertex buffer
    //
    Vertex vertices[] =
    {
        { -0.5f,  0.5f, 0.0f,  1, 0, 0, 1 },
        {  0.5f,  0.5f, 0.0f,  0, 1, 0, 1 },
        {  0.5f, -0.5f, 0.0f,  0, 0, 1, 1 },

        { -0.5f,  0.5f, 0.0f,  1, 0, 0, 1 },
        {  0.5f, -0.5f, 0.0f,  0, 0, 1, 1 },
        { -0.5f, -0.5f, 0.0f,  1, 1, 0, 1 },
    };

    D3D11_BUFFER_DESC bd = {};
    bd.Usage = D3D11_USAGE_DEFAULT;
    bd.ByteWidth = sizeof(vertices);
    bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;

    D3D11_SUBRESOURCE_DATA initData = {};
    initData.pSysMem = vertices;

    hr = m_device->CreateBuffer(&bd, &initData, &m_vertexBuffer);
    if (FAILED(hr))
        return false;

    m_camera.x = 0.0f;
    m_camera.y = 0.0f;
    m_camera.zoom = 1.0f;
    m_camera.viewWidth = 2.0f;
    m_camera.viewHeight = 2.0f;

    return true;
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


void DX11Renderer::RenderFrame()
{
    using clock = std::chrono::steady_clock;
    static auto last = clock::now();

    auto now = clock::now();
    float dt = std::chrono::duration<float>(now - last).count();
    last = now;

    UpdateCamera(dt);

    // 1. Bind the render target
    m_context->OMSetRenderTargets(1, &m_rtv, nullptr);
    m_context->RSSetViewports(1, &m_viewport);

    // 2. Clear the screen
    float clearColor[4] = { 0.1f, 0.1f, 0.1f, 1.0f };
    m_context->ClearRenderTargetView(m_rtv, clearColor);

    // 3. Set the viewport
    D3D11_VIEWPORT vp = {};
    vp.TopLeftX = 0;
    vp.TopLeftY = 0;
    
	RECT rc;
	GetClientRect(m_hwnd, &rc);
	vp.Width = static_cast<FLOAT>(rc.right - rc.left);
	vp.Height = static_cast<FLOAT>(rc.bottom - rc.top);

    vp.MinDepth = 0.0f;
    vp.MaxDepth = 1.0f;
    m_context->RSSetViewports(1, &vp);

    // 4. Bind the pipeline state
    m_context->IASetInputLayout(m_inputLayout);
    m_context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    UINT stride = sizeof(Vertex);
    UINT offset = 0;
    m_context->IASetVertexBuffers(0, 1, &m_vertexBuffer, &stride, &offset);

    m_context->VSSetShader(m_vertexShader, nullptr, 0);
    m_context->PSSetShader(m_pixelShader, nullptr, 0);

    for (int y = 0; y < 10; y++)
    {
        for (int x = 0; x < 10; x++)
        {
            float px = (x - 5) * 0.2f;
            float py = (y - 5) * 0.2f;
            DrawQuad(px, py, 0.1f);
        }
    }

    // 6. Present the frame
    m_swapChain->Present(1, 0);
}

XMFLOAT2 DX11Renderer::ScreenToWorld(int sx, int sy)
{
    float ndcX = (2.0f * sx / m_viewport.Width) - 1.0f;
    float ndcY = 1.0f - (2.0f * sy / m_viewport.Height);

    float worldX = m_camera.x + ndcX * (m_camera.viewWidth / m_camera.zoom) * 0.5f;
    float worldY = m_camera.y + ndcY * (m_camera.viewHeight / m_camera.zoom) * 0.5f;

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
}

void DX11Renderer::OnMouseMove(int x, int y)
{
    if (!m_middleDown)
        return;

    POINT current;
    current.x = x;
    current.y = y;

    float dx = float(current.x - m_lastMousePos.x);
    float dy = float(current.y - m_lastMousePos.y);

    // Convert screen movement to world movement
    float worldPerPixelX = (m_camera.viewWidth / m_camera.zoom) / m_viewport.Width;
    float worldPerPixelY = (m_camera.viewHeight / m_camera.zoom) / m_viewport.Height;

    m_camera.x -= dx * worldPerPixelX;
    m_camera.y += dy * worldPerPixelY; // invert Y

    m_lastMousePos = current;
}

void DX11Renderer::OnMouseWheel(short delta)
{
    float zoomFactor = 1.0f + (delta / 120.0f) * 0.1f;
    m_camera.zoom *= zoomFactor;

    if (m_camera.zoom < 0.1f)
        m_camera.zoom = 0.1f;
}

void DX11Renderer::Shutdown()
{
    if (m_rtv)        m_rtv->Release();
    if (m_swapChain)  m_swapChain->Release();
    if (m_context)    m_context->Release();
    if (m_device)     m_device->Release();
}
