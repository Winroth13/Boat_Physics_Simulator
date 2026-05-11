#include "core/window.h"
#include "core/engine.h"
#include "core/logger.h"

Window::Window()
	:m_hInstance(GetModuleHandle(nullptr)), mWindow(nullptr)
{
}

Window::~Window()
{
}

static ImguiWindow* sImguiWindow;
LRESULT CALLBACK WindowProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	if (sImguiWindow->ProcessMessages(hWnd, message, wParam, lParam))
		return true;

	switch (message)
	{
	case WM_CLOSE:
		DestroyWindow(hWnd);
		return 0;
	case WM_DESTROY:
		PostQuitMessage(0);
		return 0;

	default:
		break;
	}

	return DefWindowProc(hWnd, message, wParam, lParam);
}

bool Window::Create(const std::string title, const int width, const int height)
{
	mWidth = width;
	mHeight = height;

	const wchar_t* CLASS_NAME = L"Window Class";

	std::wstring titleTempStr = std::wstring(title.begin(), title.end());
	LPCWSTR wideTitleStr = titleTempStr.c_str();

	WNDCLASS wc = { };

	wc.lpfnWndProc = WindowProc;
	wc.hInstance = m_hInstance;
	wc.lpszClassName = CLASS_NAME;

	RegisterClass(&wc);

	// scale width and height by accomodating window dectorators
	RECT wr = { 0,0, mWidth, mHeight };
	AdjustWindowRect(&wr, WS_OVERLAPPEDWINDOW, FALSE);

	mWindow = CreateWindowEx(
		0, CLASS_NAME, wideTitleStr,
		WS_OVERLAPPEDWINDOW,
		CW_USEDEFAULT, CW_USEDEFAULT,
		wr.right - wr.left,
		wr.bottom - wr.top,
		nullptr, nullptr, m_hInstance, nullptr
	);

	if (mWindow == nullptr)
	{
		Logger::Error("Failed to create window");
		return false;
	}

	ShowWindow(mWindow, SW_SHOW);

	return true;
}

bool Window::CreateImguiWindow(Renderer* renderer)
{
	if (renderer == nullptr)
	{
		Logger::Error("Failed to create ImguiWindow because renderer was invalid");
		return false;
	}

	bool result = mImguiWindow.Create(this, renderer);
	sImguiWindow = &mImguiWindow;

	return result;
}

void Window::Shutdown()
{
	mImguiWindow.Shutdown();
	const wchar_t* CLASS_NAME = L"Window Class";
	UnregisterClass(CLASS_NAME, m_hInstance);
}

bool Window::PollEvents()
{
	MSG msg = {};

	while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
	{
		if (msg.message == WM_QUIT)
		{
			return false;
		}

		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

	return true;
}

void Window::BeginImguiRender()
{
	mImguiWindow.BeginRender();
}

void Window::EndImguiRender()
{
	mImguiWindow.EndRender();
}