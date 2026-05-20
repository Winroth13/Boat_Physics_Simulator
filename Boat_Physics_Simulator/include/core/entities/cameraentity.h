#pragma once
#include "core/entities/entity.h"

class CameraEntity : public Entity
{
public:
	CameraEntity();
	~CameraEntity();

    void SetFov(float fov);

protected:
	virtual void UpdateSelf(double delta) override;
	virtual void RenderSelf(RenderServer& renderServer) override;
	virtual void RenderImguiSelf() override;
};