#pragma once

#include <Windows.h>
#include <iostream>

#include "core/window.h"
#include "core/scene.h"
#include "core/renderer/renderserver.h"

class App
{
public:
	App() {};
	virtual ~App() {};

	virtual void Initialize() = 0;
	virtual void Shutdown() = 0;

	virtual void Update(double delta) = 0;
	virtual void Render(RenderServer& renderServer) = 0;
	virtual void ImguiRender(RenderServer& renderServer) = 0;

	void SetScene(Scene* scene) { mScene = scene; }

	inline bool IsRunning() const { return mIsRunning; }
	void Quit() { mIsRunning = false; }

protected:
	Scene* mScene = nullptr;

private:
	bool mIsRunning = true;
};