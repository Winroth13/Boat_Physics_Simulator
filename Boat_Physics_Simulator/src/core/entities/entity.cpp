#include "core/entities/entity.h"
#include "imgui/imgui.h"
#include "core/imguiflags.h"
#include "core/scene.h"

#include <string>

Entity::Entity()
	: Entity("Entity")
{
}

Entity::Entity(std::string name)
	:mName(name)
{
}

void Entity::Begin(RenderServer& renderServer)
{
	BeginSelf(renderServer);
}

void Entity::Update(double delta)
{
	UpdateSelf(delta);
}

void Entity::Render(RenderServer& renderServer)
{
	if (IsVisible())
	{
		RenderSelf(renderServer);
	}
}

void Entity::RenderImgui()
{
	bool isStatic = HasFlag(EntityFlags::STATIC);
	if (isStatic)
	{
		ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "This entity is static.");
		ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "Most changes here will have no effect!");
		ImGui::PushStyleColor(ImGuiCol_Header, ImVec4(1.0f, 1.0f, 0, 0.25f));
		ImGui::PushStyleColor(ImGuiCol_HeaderActive, ImVec4(1.0f, 1.0f, 0, 0.25f));
		ImGui::PushStyleColor(ImGuiCol_HeaderHovered, ImVec4(1.0f, 1.0f, 0, 0.25f));
	}

	if (ImGui::TreeNodeEx("Transform", TREE_NODE_FLAGS))
	{
		DirectX::XMFLOAT3 position = transform.GetPosition3f();
		DirectX::XMFLOAT3 scale = transform.GetScale3f();

		if (ImGui::DragFloat3("Position", &position.x, 0.1f))
		{
			transform.SetPosition(position);
		}

		DirectX::XMFLOAT3 anglesDeg = transform.GetAngles3f();
		anglesDeg.x *= (180.0f / DirectX::XM_PI);
		anglesDeg.y *= (180.0f / DirectX::XM_PI);
		anglesDeg.z *= (180.0f / DirectX::XM_PI);

		if (ImGui::DragFloat3("Angles", &anglesDeg.x, 1, 0, 0, "%.1f deg"))
		{
			DirectX::XMFLOAT3 angles = anglesDeg;
			angles.x *= (DirectX::XM_PI / 180.0f);
			angles.y *= (DirectX::XM_PI / 180.0f);
			angles.z *= (DirectX::XM_PI / 180.0f);
			transform.SetAngles(angles);
		}

		if (ImGui::DragFloat3("Scale", &scale.x, 0.1f))
		{
			transform.SetScale(scale);
		}

		if (mAttach)
		{
			ImGui::Text("Attached to: %s", mAttach->GetName().c_str());
		}
		else
		{
			ImGui::Text("Attached to: None");
		}
		ImGui::TreePop();
	}

	ImGui::Checkbox("Visible", &mVisible);

	RenderImguiSelf();

	if (isStatic)
		ImGui::PopStyleColor(3);
}

void Entity::SetName(const std::string& name)
{
	mScene->SetEntityName(this, name);
}

bool Entity::IsVisible()
{
	if (mAttach && !mAttach->IsVisible())
	{
		return false;
	}
	return mVisible;
}

DirectX::XMMATRIX Entity::GetGlobalTransform()
{
	DirectX::XMMATRIX mat = transform.GetMatrix();
	if (mAttach != nullptr)
	{
		mat *= mAttach->GetGlobalTransform();
	}
	return mat;
}

DirectX::XMFLOAT3 Entity::GetGlobalPosition()
{
	DirectX::XMVECTOR translation;
	DirectX::XMVECTOR rotation;
	DirectX::XMVECTOR scale;

	DirectX::XMMatrixDecompose(&scale, &rotation, &translation, GetGlobalTransform());

	DirectX::XMFLOAT3 position = { 0, 0, 0 };
	DirectX::XMStoreFloat3(&position, translation);

	return position;
}

DirectX::XMFLOAT3 Entity::GetGlobalAngles()
{
	/*DirectX::XMVECTOR translation;
	DirectX::XMVECTOR rotation;
	DirectX::XMVECTOR scale;

	DirectX::XMMatrixDecompose(&scale, &rotation, &translation, GetGlobalTransform());

	DirectX::XMFLOAT4 rotationQuat;
	DirectX::XMStoreFloat4(&rotationQuat, rotation);
	DirectX::XMFLOAT3 rotation3f = Transform::QuaternionToEulerAngles(rotationQuat);

	return rotation3f;*/

	DirectX::XMFLOAT3 rotation = transform.GetAngles3f();
	if (mAttach != nullptr)
	{
		DirectX::XMFLOAT3 attachRot = mAttach->GetGlobalAngles();
		rotation.x += attachRot.x;
		rotation.y += attachRot.y;
		rotation.z += attachRot.z;
	}
	return rotation;
}

DirectX::XMFLOAT3 Entity::GetGlobalScale()
{
	DirectX::XMVECTOR translation;
	DirectX::XMVECTOR rotation;
	DirectX::XMVECTOR scale;

	DirectX::XMMatrixDecompose(&scale, &rotation, &translation, GetGlobalTransform());

	DirectX::XMFLOAT3 scale3f = { 0, 0, 0 };
	DirectX::XMStoreFloat3(&scale3f, scale);

	return scale3f;
}

DirectX::XMFLOAT3 Entity::GetGlobalForwardDir()
{
	Transform globalTransform = transform;

	if (mAttach != nullptr)
	{
		DirectX::XMFLOAT3 attachAngles = mAttach->GetGlobalAngles();
		globalTransform.Rotate(
			attachAngles.x,
			attachAngles.y,
			attachAngles.z
		);
	}

	return globalTransform.GetForwardDir3f();
}

bool Entity::HasFlag(const EntityFlags flag) const
{
	return (mFlags & static_cast<uint32_t>(flag)) == static_cast<uint32_t>(flag);
}

void Entity::SetFlag(const EntityFlags flag, bool enabled)
{
	if (enabled)
	{
		mFlags |= static_cast<uint32_t>(flag);
	}
	else
	{
		mFlags &= ~static_cast<uint32_t>(flag);
	}
}
