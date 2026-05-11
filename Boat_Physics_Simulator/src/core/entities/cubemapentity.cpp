#include "core/entities/cubemapentity.h"
#include "graphics/textures/cubemaptexture.h"

#include "imgui/imgui.h"
#include "core/imguiflags.h"

#include "graphics/textures/imagetexture2d.h"

CubemapEntity::CubemapEntity(uint32_t dimensions)
	: Entity("Cubemap")
{
	mCubemapTexture = std::make_shared<CubemapTexture>(dimensions, dimensions);
	mIcon = std::make_shared<ImageTexture2D>("assets/sprites/cubemap.png");
}

CubemapEntity::~CubemapEntity()
{
}

void CubemapEntity::UpdateSelf(double delta)
{
}

void CubemapEntity::RenderSelf(RenderServer& renderServer)
{
	if (mIsDynamic || mShouldCapture)
	{
		renderServer.PushCubemap(GetGlobalPosition(), mCubemapTexture);
		mShouldCapture = false;
	}
	if (mIsDynamic)
		renderServer.PushSprite(mIcon, GetGlobalTransform(), 0.35f, {1.0f, 1.0f, 1.0f});
	else
		renderServer.PushSprite(mIcon, GetGlobalTransform(), 0.35f, { 0.5f, 0.5f, 0.5f });
}

void CubemapEntity::RenderImguiSelf()
{
	if (ImGui::TreeNodeEx("Cubemap", TREE_NODE_FLAGS))
	{
		ImGui::Checkbox("Dynamic", &mIsDynamic);

		if (!mIsDynamic)
		{
			if (ImGui::Button("Capture"))
			{
				mShouldCapture = true;
			}
		}
		ImGui::TreePop();
	}
}