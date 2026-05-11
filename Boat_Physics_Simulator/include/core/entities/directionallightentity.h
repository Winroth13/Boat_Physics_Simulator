#pragma once
#include "core/entities/entity.h"

class DirectionalLightEntity : public Entity
{
public:
	DirectionalLightEntity();
	~DirectionalLightEntity();

	void SetColor(const DirectX::XMFLOAT3 color) { mColor = color; };
	void SetIntensity(const float intensity) { mIntensity = intensity; };

	DirectX::XMFLOAT3 GetColor() const { return mColor; };
	float GetIntensity() const { return mIntensity; };

protected:
	virtual void UpdateSelf(double delta) override;
	virtual void RenderSelf(RenderServer& renderServer) override;
	virtual void RenderImguiSelf() override;

private:
	std::shared_ptr<Texture2D> mIcon;

	DirectX::XMFLOAT3 mColor = { 1.0, 1.0, 1.0 };
	float mIntensity = 1.0f;
};
