#pragma once
#include <Windows.h>

class Window;
class Renderer;

class ImguiWindow
{
public:
    ImguiWindow() {};
    ~ImguiWindow() {};

    bool ProcessMessages(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
    bool Create(Window* window, Renderer* renderer);
    void Shutdown();

    void BeginRender();
    void EndRender();
};