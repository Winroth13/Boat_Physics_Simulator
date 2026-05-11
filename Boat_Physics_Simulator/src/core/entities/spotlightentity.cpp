#include "core/entities/spotlightentity.h"

#include "imgui/imgui.h"
#include "core/imguiflags.h"

#include "graphics/textures/imagetexture2d.h"

SpotLightEntity::SpotLightEntity() : Entity("Spot Light") 
{
	mIcon = std::make_shared<ImageTexture2D>("assets/sprites/spot_light.png");
}

SpotLightEntity::~SpotLightEntity() {}

void SpotLightEntity::UpdateSelf(double delta)
{
}

void SpotLightEntity::RenderSelf(RenderServer& renderServer)
{
	renderServer.PushSpotLight(
		GetGlobalForwardDir(),
		GetGlobalPosition(),
		mColor,
		mAngle,
		mIntensity,
		mAttenuation
	);

	renderServer.PushSprite(mIcon, GetGlobalTransform(), 0.35f, mColor);
}

void SpotLightEntity::RenderImguiSelf()
{
	if (ImGui::TreeNodeEx("Light Properties", TREE_NODE_FLAGS))
	{
		ImGui::ColorEdit3("Color", &mColor.x);
		ImGui::DragFloat("Intensity", &mIntensity, 0.05f);
		ImGui::DragFloat("Attenuation", &mAttenuation, 0.02f, 0.01f, 20.0f);
		ImGui::DragFloat("Angle", &mAngle, 0.02f, 0.01f, DirectX::XM_PIDIV2 - 0.01f);

		DirectX::XMFLOAT3 dir = transform.GetForwardDir3f();
		ImGui::Text("Direction: (%.3f, %.3f, %.3f)", dir.x, dir.y, dir.z);
		ImGui::TreePop();
	}
}