
#pragma once

#include <windows.h>

class Window
{
public:
    bool Create(int width, int height, const wchar_t* title);
    HWND GetHWND() const { return m_hwnd; }

private:
    HWND m_hwnd = nullptr;
};
