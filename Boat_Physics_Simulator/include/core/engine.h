#pragma once
#include "app.h"
#include "core/window.h"
#include "core/renderer/renderer.h"
#include "core/scene.h"

#include <chrono>

class Engine
{
public:
	void Run(App* app, const std::string& title, const int width, const int height);
	void Shutdown();

	static const uint32_t GetTicks();

private:
	bool Initialize(const std::string& title, const int width, const int height);
	void Update(double delta);
	void Render();

	const double GetDelta();

private:
	Window mWindow;
	Renderer mRenderer;
	Scene mScene;

	App* mApp = nullptr;

	std::chrono::steady_clock::time_point mLastTime;
	static uint32_t mTicks;
};
