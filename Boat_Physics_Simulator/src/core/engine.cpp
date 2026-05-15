#include "core/engine.h"
#include "core/logger.h"

uint32_t Engine::mTicks = 0;
double Engine::mElapsedTime = 0;

void Engine::Run(App* app, const std::string& title, const int width, const int height)
{
	mApp = app;
	mApp->SetScene(&mScene);

	if (Initialize(title, width, height))
	{
		while (mApp->IsRunning())
		{
			double delta = GetDelta();
			Update(delta);

			Render();
		}
	}
	Shutdown();
}

void Engine::Shutdown()
{
	mScene.Shutdown();
	mApp->Shutdown();
	mRenderer.Shutdown();
	mWindow.Shutdown();
}

bool Engine::Initialize(const std::string& title, const int width, const int height)
{
	if (!mWindow.Create(title, width, height))
	{
		Logger::Error("Failed to create window");
		return false;
	}

	if (!mRenderer.Create({ 0.082f, 0.769f, 0.996f, 0.0f }, &mWindow))
	{
		Logger::Error("Failed to create renderer");
		return false;
	}

	if (!mWindow.CreateImguiWindow(&mRenderer))
	{
		Logger::Error("Failed to imgui window");
		return false;
	}

	mApp->Initialize();
	mScene.Begin(mRenderer.GetRenderServer());
	mRenderer.BakeStaticGeometry();

	mLastTime = std::chrono::steady_clock::now();
	return true;
}

void Engine::Update(double delta)
{
	if (!mWindow.PollEvents())
	{
		mApp->Quit();
	}

	mScene.Update(delta);
	mApp->Update(delta);

	mTicks++;
	mElapsedTime += delta;
}

void Engine::Render()
{
	mRenderer.BeginRender();

	/* Scene */
	{
		mApp->Render(mRenderer.GetRenderServer());
		mScene.Render(mRenderer.GetRenderServer());
		mRenderer.Render();
	}

	/* Imgui */
	{
		mRenderer.BeginForward();
		mRenderer.RenderForward();
		mWindow.BeginImguiRender();
		mApp->ImguiRender(mRenderer.GetRenderServer());
		mWindow.EndImguiRender();
		mRenderer.EndForward();
	}

	mRenderer.PresentRender();
}

const double Engine::GetDelta()
{
	auto currentTime = std::chrono::steady_clock::now();

	double delta = std::chrono::duration<double>(currentTime - mLastTime).count();
	mLastTime = currentTime;

	return delta;
}

const uint32_t Engine::GetTicks()
{
	return mTicks;
}

const double Engine::GetElapsedTime()
{
	return mElapsedTime;
}