#pragma once

#include <d3d11.h>
#include <DirectXMath.h>
#include <windows.h>

using namespace DirectX;

class Camera2D
{
public:
	Camera2D();
	~Camera2D();

    XMFLOAT2 ScreenToWorld(int sx, int sy);
    void UpdateCamera(float dt);

    void OnMouseMove(int inX, int inY);
    void OnMouseWheel(short delta, HWND hwnd);

	float GetX() const { return x; }
	float GetY() const { return y; }
	float GetZoom() const { return zoom; }
	float GetViewWidth() const { return viewWidth; }
	float GetViewHeight() const { return viewHeight; }

    D3D11_VIEWPORT viewport;
    POINT m_lastMousePos = { 0, 0 };
    bool m_middleDown = false;

private:
    float x;
    float y;
    float zoom;

    float viewWidth;
    float viewHeight;
};
