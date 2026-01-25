
#include "../../../Engine/src/Core/Rendering/Camera.h"
#include "../../../Engine/src/Core/Rendering/Renderer.h"
#include "../../../Engine/src/Core/Rendering/Window.h"
#include "../../../Engine/src/Core/Structures/Vertex.h"

#include <windows.h>
#include <windowsx.h>

int main()
{
    Window window;
    window.Create(1280, 720, L"Systems Playground");

    DX11Renderer renderer;
    renderer.Init(window.GetHWND());

    MSG msg = {};
    bool running = true;

    while (running)
    {
        while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
        {
            switch (msg.message)
            {
                case WM_MBUTTONDOWN:
                    SetCapture(window.GetHWND());
                    renderer.GetCamera().m_middleDown = true;
                    GetCursorPos(&renderer.GetCamera().m_lastMousePos);
                    break;

                case WM_MBUTTONUP:
                    ReleaseCapture();
                    renderer.GetCamera().m_middleDown = false;
                    break;

                case WM_MOUSEWHEEL:
                {
                    short delta = GET_WHEEL_DELTA_WPARAM(msg.wParam);
                    renderer.GetCamera().OnMouseWheel(delta, window.GetHWND());
                    break;
                }
            
                case WM_MOUSEMOVE:
                {
                    int x = GET_X_LPARAM(msg.lParam);
                    int y = GET_Y_LPARAM(msg.lParam);
                    renderer.GetCamera().OnMouseMove(x, y);
                    break;
                }

                case WM_QUIT:
				    running = false;
                    break;

                default:
                    TranslateMessage(&msg);
                    DispatchMessage(&msg);
                    break;
            }
        }

        renderer.RenderFrame();
    }

    renderer.Shutdown();
}
