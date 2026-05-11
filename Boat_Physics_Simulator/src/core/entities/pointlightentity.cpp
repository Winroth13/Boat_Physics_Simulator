#include "core/entities/pointlightentity.h"

#include "imgui/imgui.h"
#include "core/imguiflags.h"

#include "graphics/textures/imagetexture2d.h"
#include "core/logger.h"

PointLightEntity::PointLightEntity() : Entity("Point Light") 
{
	mIcon = std::make_shared<ImageTexture2D>("assets/sprites/point_light.png");
}

PointLightEntity::~PointLightEntity() {}

void PointLightEntity::UpdateSelf(double delta)
{
}

void PointLightEntity::RenderSelf(RenderServer& renderServer)
{
	bool updateShadows = true;

	if (!mDynamicShadows)
	{
		if (mShouldUpdateShadows)
		{
			updateShadows = true;
			mShouldUpdateShadows = false;
		}
		else
		{
			updateShadows = false;
		}
	}

	renderServer.PushPointLight(GetGlobalPosition(), mColor, mAttenuation, mIntensity, updateShadows);
	renderServer.PushSprite(mIcon, GetGlobalTransform(), 0.35f, mColor);
}

void PointLightEntity::RenderImguiSelf()
{
	if (ImGui::TreeNodeEx("Light Properties", TREE_NODE_FLAGS))
	{
		ImGui::ColorEdit3("Color", &mColor.x);
		ImGui::DragFloat("Attenuation", &mAttenuation, 0.02f, 0.01f, 20.0f);
		ImGui::DragFloat("Intensity", &mIntensity, 0.05f);
		ImGui::TreePop();
	}

	if (ImGui::TreeNodeEx("Shadows", TREE_NODE_FLAGS))
	{
		bool toggled = mDynamicShadows;
		if (ImGui::Checkbox("Dynamic Shadows", &toggled))
		{
			mDynamicShadows = toggled;
		}
		ImGui::TreePop();
	}
}