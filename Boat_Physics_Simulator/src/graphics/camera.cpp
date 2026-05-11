#include "graphics/camera.h"
#include <DirectXMath.h>

using namespace DirectX;

Camera::Camera()
{
}

Camera::~Camera()
{
}

float Camera::GetNearZ() const
{
	return mNearZ;
}

float Camera::GetFarZ() const
{
	return mFarZ;
}

float Camera::GetAspect() const
{
	return mAspect;
}

float Camera::GetFovY() const
{
	return mFovY;
}

float Camera::GetFovX() const
{
	float halfWidth = 0.5f * GetNearWindowWidth();
	return 2.0f * atanf(halfWidth / mNearZ);
}

float Camera::GetNearWindowWidth() const
{
	return mAspect * mNearWindowHeight;
}

float Camera::GetNearWindowHeight() const
{
	return mNearWindowHeight;
}

float Camera::GetFarWindowWidth() const
{
	return mAspect * mFarWindowHeight;
}

float Camera::GetFarWindowHeight() const
{
	return mFarWindowHeight;
}

void Camera::SetPerspectiveLens(float fovY, float aspect, float zNear, float zFar)
{
	mFovY = fovY;
	mAspect = aspect;
	mNearZ = zNear;
	mFarZ = zFar;

	mNearWindowHeight = 2.0f * mNearZ * tanf(0.5f * mFovY);
	mFarWindowHeight = 2.0f * mFarZ * tanf(0.5f * mFovY);

	XMMATRIX p = XMMatrixPerspectiveFovLH(mFovY, mAspect, mNearZ, mFarZ);
	XMStoreFloat4x4(&mProj, p);
}

void Camera::SetOrthographicLens(float width, float height, float zNear, float zFar)
{
	mFovY = 0;
	mAspect = width / height;
	mNearZ = zNear;
	mFarZ = zFar;

	mNearWindowHeight = height;
	mFarWindowHeight = height;

	XMMATRIX o = XMMatrixOrthographicLH(width, height, zNear, zFar);
	XMStoreFloat4x4(&mProj, o);
}

XMMATRIX Camera::GetView() const
{
	return XMLoadFloat4x4(&mView);
}

XMMATRIX Camera::GetProj() const
{
	return XMLoadFloat4x4(&mProj);
}

XMMATRIX Camera::GetViewProj() const
{
	return GetView() * GetProj();
}

XMFLOAT4X4 Camera::GetView4x4f() const
{
	return mView;
}

XMFLOAT4X4 Camera::GetProj4x4f() const
{
	return mProj;
}

void Camera::UpdateViewMatrix()
{
	XMFLOAT3 right = transform.GetRightDir3f();
	XMFLOAT3 up = transform.GetUpDir3f();
	XMFLOAT3 look = transform.GetForwardDir3f();
	XMFLOAT3 pos = transform.GetPosition3f();

	XMVECTOR R = XMLoadFloat3(&right);
	XMVECTOR U = XMLoadFloat3(&up);
	XMVECTOR L = XMLoadFloat3(&look);
	XMVECTOR P = XMLoadFloat3(&pos);

	L = XMVector3Normalize(L);
	U = XMVector3Normalize(XMVector3Cross(L, R));

	R = XMVector3Cross(U, L);

	float x = -XMVectorGetX(XMVector3Dot(P, R));
	float y = -XMVectorGetX(XMVector3Dot(P, U));
	float z = -XMVectorGetX(XMVector3Dot(P, L));

	XMStoreFloat3(&right, R);
	XMStoreFloat3(&up, U);
	XMStoreFloat3(&look, L);

	mView(0, 0) = right.x;
	mView(1, 0) = right.y;
	mView(2, 0) = right.z;
	mView(3, 0) = x;

	mView(0, 1) = up.x;
	mView(1, 1) = up.y;
	mView(2, 1) = up.z;
	mView(3, 1) = y;

	mView(0, 2) = look.x;
	mView(1, 2) = look.y;
	mView(2, 2) = look.z;
	mView(3, 2) = z;

	mView(0, 3) = 0.0f;
	mView(1, 3) = 0.0f;
	mView(2, 3) = 0.0f;
	mView(3, 3) = 1.0f;
}

BoundingFrustum Camera::GetBoundingFrustum()
{
	BoundingFrustum boundingFrustum = {};
	BoundingFrustum::CreateFromMatrix(boundingFrustum, GetProj());
	boundingFrustum.Origin = transform.GetPosition3f();

	XMFLOAT3 angles = transform.GetAngles3f();
	XMVECTOR orientation = DirectX::XMQuaternionRotationRollPitchYawFromVector(
		DirectX::XMLoadFloat3(&angles)
	);

	XMFLOAT4 orientationf;
	DirectX::XMStoreFloat4(&orientationf, orientation);
	boundingFrustum.Orientation = orientationf;

	boundingFrustum.Near = mNearZ;
	boundingFrustum.Far = mFarZ;

	return boundingFrustum;
}