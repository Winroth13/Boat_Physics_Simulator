#pragma once
#include "core/entities/entity.h"


class EnviromentEntity : public Entity
{
public:
	EnviromentEntity();
	~EnviromentEntity();

	void SetAmbientColor(float r, float g, float b);

protected:
	virtual void UpdateSelf(double delta) override;
	virtual void RenderSelf(RenderServer& renderServer) override;
	virtual void RenderImguiSelf() override;

};