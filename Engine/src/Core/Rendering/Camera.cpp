
#include "Camera.h"

#include <algorithm>
#include <cmath>
#include <DirectXMath.h>
#include <windows.h>

using namespace DirectX;

Camera2D::Camera2D()
	: viewport()
	, m_lastMousePos{ 0, 0 }
	, m_middleDown(false)
    , x(0.0f)
    , y(0.0f)
    , zoom(1.0f)
    , viewWidth(30.0f)
    , viewHeight(30.0f)

{
}

Camera2D::~Camera2D()
{
    viewHeight = 0.0f;
    viewWidth = 0.0f;
    zoom = 0.0f;
    y = 0.0f;
    x = 0.0f;
    m_middleDown = false;
	m_lastMousePos = { 0, 0 };
	viewport = {};
}

XMFLOAT2 Camera2D::ScreenToWorld(int sx, int sy)
{
    float ndcX = (2.0f * sx / viewport.Width) - 1.0f;
    float ndcY = 1.0f - (2.0f * sy / viewport.Height);

    float worldWidth = viewWidth / zoom;
    float worldHeight = viewHeight / zoom;

    float worldX = x + ndcX * (worldWidth * 0.5f);
    float worldY = y + ndcY * (worldHeight * 0.5f);

    return { worldX, worldY };
}

void Camera2D::UpdateCamera(float dt)
{
    const float moveSpeed = 10.0f;   // world units per second
    const float zoomSpeed = 1.5f;    // zoom multiplier per second

    // Pan
    if (GetAsyncKeyState(VK_LEFT) & 0x8000)
        x -= moveSpeed * dt;

    if (GetAsyncKeyState(VK_RIGHT) & 0x8000)
        x += moveSpeed * dt;

    if (GetAsyncKeyState(VK_UP) & 0x8000)
        y += moveSpeed * dt;

    if (GetAsyncKeyState(VK_DOWN) & 0x8000)
        y -= moveSpeed * dt;

    // Zoom in/out
    if (GetAsyncKeyState('Q') & 0x8000)
        zoom *= (1.0f + zoomSpeed * dt);

    if (GetAsyncKeyState('E') & 0x8000)
        zoom *= (1.0f - zoomSpeed * dt);

    // Clamp zoom so it never flips or goes negative
    if (zoom < 0.1f)
        zoom = 0.1f;

    if (!std::isfinite(x)) x = 0.0f;
    if (!std::isfinite(y)) y = 0.0f;
    if (!std::isfinite(zoom)) zoom = 1.0f;
}

void Camera2D::OnMouseMove(int inX, int inY)
{
    if (!m_middleDown)
    {
        m_lastMousePos.x = inX;
        m_lastMousePos.y = inY;
        return;
    }

    float dx = x - m_lastMousePos.x;
    float dy = y - m_lastMousePos.y;

    m_lastMousePos.x = inX;
    m_lastMousePos.y = inY;

    // Convert pixel delta to world delta
    float worldWidth = viewWidth /  zoom;
    float worldHeight = viewHeight / zoom;

    float worldPerPixelX = worldWidth / viewWidth;
    float worldPerPixelY = worldHeight / viewHeight;

    // Move camera opposite to mouse drag
    x -= dx * worldPerPixelX;
    y += dy * worldPerPixelY; // + because screen Y is inverted
}

void Camera2D::OnMouseWheel(short delta, HWND hwnd)
{
    // Get cursor in client space
    POINT p;
    GetCursorPos(&p);
    ScreenToClient(hwnd, &p);

    // World position before zoom
    XMFLOAT2 before = ScreenToWorld(p.x, p.y);

    // Logarithmic zoom
    float zoomFactor = std::exp((delta / 120.0f) * 0.1f);
    zoom *= zoomFactor;

    // Clamp zoom to safe range
    zoom = std::clamp(zoom, 0.5f, 4.0f);

    // World position after zoom
    XMFLOAT2 after = ScreenToWorld(p.x, p.y);

    // Shift camera so the point stays under the cursor
    x += (before.x - after.x);
    y += (before.y - after.y);
}
