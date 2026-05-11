#pragma once
#include <iostream>
#include <vector>
#include <DirectXMath.h>

#include "math/transform.h"
#include "core/renderer/renderserver.h"

class Scene;

enum class EntityFlags : uint32_t
{
	NONE = 0,
	STATIC = 1,
};

class Entity
{
	friend class Scene;

public:
	Entity();
	Entity(std::string name);

	virtual ~Entity() = default;

	void Begin(RenderServer& renderServer);
	void Update(double delta);
	void Render(RenderServer& renderServer);
	void RenderImgui();

	std::string& GetName() { return mName; }
	void SetName(const std::string& name);

	bool IsVisible();
	void SetVisible(const bool visible) { mVisible = visible; }
	DirectX::XMMATRIX GetGlobalTransform();
	DirectX::XMFLOAT3 GetGlobalPosition();
	DirectX::XMFLOAT3 GetGlobalAngles();
	DirectX::XMFLOAT3 GetGlobalScale();
	DirectX::XMFLOAT3 GetGlobalForwardDir();

	void Attach(Entity* entity) { mAttach = entity; }
	Entity* GetAttachEntity() { return mAttach; }
	const bool HasAttach() { return mAttach != nullptr; }

	bool HasFlag(const EntityFlags flag) const;
	void SetFlag(const EntityFlags flag, bool enabled);

public:
	Transform transform;

protected:
	Scene& GetScene() { return *mScene; }

	virtual void BeginSelf(RenderServer& renderServer) {};
	virtual void UpdateSelf(double delta) {};
	virtual void RenderSelf(RenderServer& renderServer) {};
	virtual void RenderImguiSelf() {};

	std::string mName;
	bool mVisible = true;
	uint32_t mFlags = 0;

	Entity* mAttach = nullptr;

private:
	Scene* mScene = nullptr;
};