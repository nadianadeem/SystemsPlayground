
#include "../../../Engine/src/Core/Window.h"
#include "../../../Engine/src/Core/Renderer.h"
#include "../../../Engine/src/Core/Vertex.h"

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
            if (msg.message == WM_QUIT)
                running = false;

            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }

        renderer.RenderFrame();
    }

    renderer.Shutdown();
}
