#pragma once
#include "core/entities/entity.h"

class CubemapTexture;

class CubemapEntity : public Entity
{
public:
	CubemapEntity(uint32_t dimensions);
	~CubemapEntity() override;

	void SetDynamic(bool isDynamic) { mIsDynamic = isDynamic; }
	std::shared_ptr<CubemapTexture> GetCubemapTexture() { return mCubemapTexture; }

protected:
	virtual void UpdateSelf(double delta) override;
	virtual void RenderSelf(RenderServer& renderServer) override;
	virtual void RenderImguiSelf() override;

private:
	std::shared_ptr<Texture2D> mIcon;

	bool mIsDynamic = false;
	bool mShouldCapture = true;
	std::shared_ptr<CubemapTexture> mCubemapTexture = nullptr;
};