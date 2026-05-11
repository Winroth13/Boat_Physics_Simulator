#pragma once
#include "core/entities/entity.h"

class ParticleSystem;

class ParticleEntity : public Entity
{
public:
	ParticleEntity(uint32_t count);
	~ParticleEntity() override;

	std::shared_ptr<ParticleSystem> GetParticleSystem() { return mParticleSystem; }

protected:
	virtual void UpdateSelf(double delta) override;
	virtual void RenderSelf(RenderServer& renderServer) override;
	virtual void RenderImguiSelf() override;

private:
	std::shared_ptr<Texture2D> mIcon;

	std::shared_ptr<ParticleSystem> mParticleSystem = nullptr;
};