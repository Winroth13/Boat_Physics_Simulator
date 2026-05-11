#include "core/scene.h"

Scene::Scene()
{
    // Give some nice defaults
    constexpr float fov = DirectX::XMConvertToRadians(80);
    mCamera.SetPerspectiveLens(fov, 16.f / 9.f, 0.1f, 100.0f); // TODO: This assumes the aspect ratio will always be 16:9
}

void Scene::Begin(RenderServer& renderServer)
{
    for (std::unique_ptr<Entity>& e : mEntities)
    {
        e->Begin(renderServer);
    }
}

void Scene::Update(double delta)
{
    for (std::unique_ptr<Entity>& e : mEntities)
    {
        e->Update(delta);
    }
}

void Scene::Render(RenderServer& renderServer)
{
    mCamera.UpdateViewMatrix();
    renderServer.UpdateCamera(
        mCamera.GetViewProj(),
        mCamera.GetView(),
        mCamera.transform.GetPosition3f()
    );

    if (!mIsFrustumLocked)
    {
        renderServer.UpdateFrustum(
            mCamera.GetBoundingFrustum()
        );
    }

    for (std::unique_ptr<Entity>& e : mEntities)
    {
        e->Render(renderServer);
    }
}

void Scene::Shutdown()
{
    mEntities.clear();
}

std::vector<std::unique_ptr<Entity>>& Scene::GetEntities()
{
    return mEntities;
}