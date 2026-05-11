#pragma once
#include "core/entities/entity.h"

#include <memory>

class PointLightEntity : public Entity
{
public:
	PointLightEntity();
	~PointLightEntity();

	void SetColor(const DirectX::XMFLOAT3 color) { mColor = color; };
	void SetAttenuation(const float attenuation) { mAttenuation = attenuation; };
	void SetIntensity(const float intensity) { mIntensity = intensity; };
	void SetDynamicShadows(const bool enabled) { mDynamicShadows = enabled; }

	DirectX::XMFLOAT3 GetColor() const { return mColor; };
	float GetAttenuation() const { return mAttenuation; };
	float GetIntensity() const { return mIntensity; };
	bool HasDynamicShadows() const { return mDynamicShadows; }

protected:
	virtual void UpdateSelf(double delta) override;
	virtual void RenderSelf(RenderServer& renderServer) override;
	virtual void RenderImguiSelf() override;

private:
	std::shared_ptr<Texture2D> mIcon;

	DirectX::XMFLOAT3 mColor = { 1.0, 1.0, 1.0 };
	float mAttenuation = 1.0f;
	float mIntensity = 1.0f;
	bool mDynamicShadows = true;
	bool mShouldUpdateShadows = true;
};
