#include "core/entities/directionallightentity.h"

#include "imgui/imgui.h"
#include "core/imguiflags.h"

#include "graphics/textures/imagetexture2d.h"

DirectionalLightEntity::DirectionalLightEntity() : Entity("Directional Light") 
{
	mIcon = std::make_shared<ImageTexture2D>("assets/sprites/directional_light.png");
}

DirectionalLightEntity::~DirectionalLightEntity() {}

void DirectionalLightEntity::UpdateSelf(double delta)
{
}

void DirectionalLightEntity::RenderSelf(RenderServer& renderServer)
{
	renderServer.PushDirectionalLight(transform.GetForwardDir3f(), mColor, mIntensity);

	renderServer.PushSprite(mIcon, GetGlobalTransform(), 0.35f, mColor);
}

void DirectionalLightEntity::RenderImguiSelf()
{
	if (ImGui::TreeNodeEx("Light Properties", TREE_NODE_FLAGS))
	{
		ImGui::ColorEdit3("Color", &mColor.x);
		ImGui::DragFloat("Intensity", &mIntensity, 0.01f, 0.01f, 20.0f);

		// Directiona light always uses its local coordinates
		DirectX::XMFLOAT3 dir = transform.GetForwardDir3f();
		ImGui::Text("Direction: (%.3f, %.3f, %.3f)", dir.x, dir.y, dir.z);
		ImGui::TreePop();
	}
}