#pragma once
#include "core/entities/entity.h"

class SpotLightEntity : public Entity
{
public:
	SpotLightEntity();
	~SpotLightEntity();

	void SetColor(const DirectX::XMFLOAT3 color) { mColor = color; };
	void SetAttenuation(const float attenuation) { mAttenuation = attenuation; };
	void SetIntensity(const float intensity) { mIntensity = intensity; };
	void SetAngle(const float angle) { mAngle = angle; };

	DirectX::XMFLOAT3 GetColor() const { return mColor; };
	float GetAttenuation() const { return mAttenuation; };
	float GetIntensity() const { return mIntensity; };
	float GetAngle() const { return mAngle; };

protected:
	virtual void UpdateSelf(double delta) override;
	virtual void RenderSelf(RenderServer& renderServer) override;
	virtual void RenderImguiSelf() override;

private:
	std::shared_ptr<Texture2D> mIcon;

	DirectX::XMFLOAT3 mColor = { 1.0, 1.0, 1.0 };
	float mAttenuation = 1.0f;
	float mIntensity = 1.0f;
	float mAngle = 3.14f / 4;
};
