#pragma once
#include "math/transform.h"
#include "core/logger.h"
#include "DirectXCollision.h"

class Camera
{
public:
	Camera();
	~Camera();

	Transform transform;

	float GetNearZ() const;
	float GetFarZ() const;
	float GetAspect() const;
	float GetFovY() const;
	float GetFovX() const;

	float GetNearWindowWidth() const;
	float GetNearWindowHeight() const;
	float GetFarWindowWidth() const;
	float GetFarWindowHeight() const;

	void SetPerspectiveLens(float fovY, float aspect, float zNear, float zFar);
	void SetOrthographicLens(float width, float height, float zNear, float zFar);

	DirectX::XMMATRIX GetView() const;
	DirectX::XMMATRIX GetProj() const;
	DirectX::XMMATRIX GetViewProj() const;

	DirectX::XMFLOAT4X4 GetView4x4f() const;
	DirectX::XMFLOAT4X4 GetProj4x4f() const;

	void UpdateViewMatrix();

	DirectX::BoundingFrustum GetBoundingFrustum();

private:
	float mNearZ = 0.0f;
	float mFarZ = 0.0f;
	float mAspect = 0.0f;
	float mFovY = 0.0f;
	float mNearWindowHeight = 0.0f;
	float mFarWindowHeight = 0.0f;

	bool mViewDirty = true;

	DirectX::XMFLOAT4X4 mView;
	DirectX::XMFLOAT4X4 mProj;
};