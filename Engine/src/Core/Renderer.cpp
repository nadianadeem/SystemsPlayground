
#include "Renderer.h"
#include "Vertex.h"
#include "TransformCB.h"

#include <windows.h>
#include <d3d11.h>
#include <dxgi.h>
#include <d3dcompiler.h>

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

    return true;
}

void DX11Renderer::DrawQuad(float x, float y, float scale)
{
    // Build transform
    TransformCB cb;
    MakeTransform(x, y, scale, cb);

    // Upload to GPU
    m_context->UpdateSubresource(m_transformCB, 0, nullptr, &cb, 0, 0);

    // Bind constant buffer
    m_context->VSSetConstantBuffers(0, 1, &m_transformCB);

    // Issue draw call
    m_context->Draw(6, 0);
}

void DX11Renderer::RenderFrame()
{
    // 1. Bind the render target
    m_context->OMSetRenderTargets(1, &m_rtv, nullptr);

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

    TransformCB cb;
    MakeTransform(0.0f, 0.0f, 0.5f, cb); // center, half size

    m_context->UpdateSubresource(m_transformCB, 0, nullptr, &cb, 0, 0);
    m_context->VSSetConstantBuffers(0, 1, &m_transformCB);

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


void DX11Renderer::Shutdown()
{
    if (m_rtv)        m_rtv->Release();
    if (m_swapChain)  m_swapChain->Release();
    if (m_context)    m_context->Release();
    if (m_device)     m_device->Release();
}
