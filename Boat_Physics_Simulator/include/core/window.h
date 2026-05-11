#pragma once

#include <Windows.h>
#include <string>

#include "core/imguiwindow.h"
#include "core/renderer/renderer.h"

LRESULT CALLBACK WindowProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);

struct WindowProperties
{
	std::string title;
	int x, y, w, h;
	float aspectRatio;
};

class Window
{
public:
	Window();
	~Window();

	bool Create(const std::string title, const int width, const int height);
	bool CreateImguiWindow(Renderer* renderer);

	void Shutdown();
	bool PollEvents();

	void BeginImguiRender();
	void EndImguiRender();

	int Height() { return mHeight; }
	int Width() { return mWidth; }

	HWND GetWindowsWindow() { return mWindow; }
	ImguiWindow& GetImguiWindow() { return mImguiWindow; }

private:
	int mWidth = 0;
	int mHeight = 0;

	ImguiWindow mImguiWindow;
	HWND mWindow;
	HINSTANCE m_hInstance;
};