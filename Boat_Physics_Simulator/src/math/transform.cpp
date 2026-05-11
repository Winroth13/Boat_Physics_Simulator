#include "math/transform.h"
#include "core/logger.h"
#include "math/mathfunctions.h"

using namespace DirectX;

/* Get/Set Position */
XMVECTOR Transform::GetPosition() const
{
	return XMLoadFloat3(&mPosition);
}

XMFLOAT3 Transform::GetPosition3f()
{
	return mPosition;
}

void Transform::SetPosition(const float x, const float y, const float z)
{
	mPosition = DirectX::XMFLOAT3(x, y, z);
	mIsDirty = true;
}

void Transform::SetPosition(const XMFLOAT3& position)
{
	mPosition = position;
	mIsDirty = true;
}

void Transform::SetPosition(const DirectX::XMVECTOR position)
{
	DirectX::XMStoreFloat3(&mPosition, position);
	mIsDirty = true;
}

void Transform::Translate(const float x, const float y, const float z)
{
	mPosition.x += x;
	mPosition.y += y;
	mPosition.z += z;
	mIsDirty = true;
}

void Transform::Translate(const DirectX::XMFLOAT3 position)
{
	mPosition.x += position.x;
	mPosition.y += position.y;
	mPosition.z += position.z;
	mIsDirty = true;
}

void Transform::Translate(const DirectX::XMVECTOR position)
{
	XMFLOAT3 positionf = { 0, 0, 0 };
	DirectX::XMStoreFloat3(&positionf, position);
	Translate(positionf);
}

/* Get/Set Angles */
XMVECTOR Transform::GetAngles() const
{
	return XMLoadFloat3(&mAngles);
}

XMFLOAT3 Transform::GetAngles3f() const
{
	return mAngles;
}

void Transform::SetAngles(const float pitch, const float yaw, const float roll)
{
	mAngles = XMFLOAT3(pitch, yaw, roll);
	mIsDirty = true;
}

void Transform::SetAngles(const XMFLOAT3& angles)
{
	mAngles = angles;
	mIsDirty = true;
}

void Transform::SetAngles(const DirectX::XMVECTOR angles)
{
	DirectX::XMStoreFloat3(&mAngles, angles);
	mIsDirty = true;
}

void Transform::SetPitch(const float angle)
{
	mAngles.x = angle;
	mIsDirty = true;
}

void Transform::SetYaw(const float angle)
{
	mAngles.y = angle;
	mIsDirty = true;
}

void Transform::SetRoll(const float angle)
{
	mAngles.z = angle;
	mIsDirty = true;
}

void Transform::Rotate(const float pitch, const float yaw, const float roll)
{
	mAngles.x += pitch;
	mAngles.y += yaw;
	mAngles.z += roll;
	mIsDirty = true;
}

void Transform::Rotate(const DirectX::XMFLOAT3 angles)
{
	mAngles.x += angles.x;
	mAngles.y += angles.y;
	mAngles.z += angles.z;
	mIsDirty = true;
}

void Transform::Rotate(const DirectX::XMVECTOR angles)
{
	XMFLOAT3 anglesf = { 0, 0, 0 };
	DirectX::XMStoreFloat3(&anglesf, angles);
	Translate(anglesf);
}

/* Get/Set Scale */
XMVECTOR Transform::GetScale() const
{
	return XMLoadFloat3(&mScale);
}

XMFLOAT3 Transform::GetScale3f() const
{
	return mScale;
}

void Transform::SetScale(const float x, const float y, const float z)
{
	mScale = XMFLOAT3(x, y, z);
	mIsDirty = true;
}

void Transform::SetScale(const XMFLOAT3& scale)
{
	mScale = scale;
	mIsDirty = true;
}

void Transform::SetScale(const DirectX::XMVECTOR scale)
{
	DirectX::XMStoreFloat3(&mScale, scale);
	mIsDirty = true;
}

void Transform::Scale(const float x, const float y, const float z)
{
	mScale.x += x;
	mScale.y += y;
	mScale.z += z;
	mIsDirty = true;
}

void Transform::Scale(const DirectX::XMFLOAT3 scale)
{
	mScale.x += scale.x;
	mScale.y += scale.y;
	mScale.z += scale.z;
	mIsDirty = true;
}

void Transform::Scale(const DirectX::XMVECTOR scale)
{
	XMFLOAT3 scalef = { 0, 0, 0 };
	DirectX::XMStoreFloat3(&scalef, scale);
	Translate(scalef);
}

/* Get Directions */
XMVECTOR Transform::GetRightDir() const
{
	XMMATRIX rotationMatrix = XMMatrixRotationRollPitchYawFromVector(XMLoadFloat3(&mAngles));
	XMFLOAT4X4 rotationMatrixf;

	XMStoreFloat4x4(&rotationMatrixf, rotationMatrix);

	return { rotationMatrixf._11, rotationMatrixf._12, rotationMatrixf._13 };
}

XMFLOAT3 Transform::GetRightDir3f()
{
	XMMATRIX rotationMatrix = XMMatrixRotationRollPitchYawFromVector(XMLoadFloat3(&mAngles));
	XMFLOAT4X4 rotationMatrixf;

	XMStoreFloat4x4(&rotationMatrixf, rotationMatrix);

	return { rotationMatrixf._11, rotationMatrixf._12, rotationMatrixf._13 };
}

XMVECTOR Transform::GetUpDir() const
{
	XMMATRIX rotationMatrix = XMMatrixRotationRollPitchYawFromVector(XMLoadFloat3(&mAngles));
	XMFLOAT4X4 rotationMatrixf;

	XMStoreFloat4x4(&rotationMatrixf, rotationMatrix);

	return { rotationMatrixf._21, rotationMatrixf._22, rotationMatrixf._23 };
}

XMFLOAT3 Transform::GetUpDir3f()
{
	XMMATRIX rotationMatrix = XMMatrixRotationRollPitchYawFromVector(XMLoadFloat3(&mAngles));
	XMFLOAT4X4 rotationMatrixf;

	XMStoreFloat4x4(&rotationMatrixf, rotationMatrix);

	return { rotationMatrixf._21, rotationMatrixf._22, rotationMatrixf._23 };
}

XMVECTOR Transform::GetForwardDir() const
{
	XMMATRIX rotationMatrix = XMMatrixRotationRollPitchYawFromVector(XMLoadFloat3(&mAngles));
	XMFLOAT4X4 rotationMatrixf;

	XMStoreFloat4x4(&rotationMatrixf, rotationMatrix);

	return { rotationMatrixf._31, rotationMatrixf._32, rotationMatrixf._33 };
}

XMFLOAT3 Transform::GetForwardDir3f()
{
	XMMATRIX rotationMatrix = XMMatrixRotationRollPitchYawFromVector(XMLoadFloat3(&mAngles));
	XMFLOAT4X4 rotationMatrixf;

	XMStoreFloat4x4(&rotationMatrixf, rotationMatrix);

	return { rotationMatrixf._31, rotationMatrixf._32, rotationMatrixf._33 };
}

/* Define transform space via LookAt parameters */
void Transform::LookAt(
	DirectX::FXMVECTOR position,
	DirectX::FXMVECTOR target,
	DirectX::FXMVECTOR up
)
{
	mAngles = AnglesFromMatrix(
		XMMatrixLookAtLH(
			position,
			target,
			up
		)
	);

	mIsDirty = false;
}

void Transform::LookAt(
	const DirectX::XMFLOAT3& position,
	const DirectX::XMFLOAT3& target,
	const DirectX::XMFLOAT3& up
)
{
	LookAt(
		XMLoadFloat3(&position),
		XMLoadFloat3(&target),
		XMLoadFloat3(&up)
	);
}

void Transform::LookAt(DirectX::FXMVECTOR target)
{
	LookAt(GetPosition(), target, GetUpDir());
}

void Transform::LookAt(const DirectX::XMFLOAT3& target)
{
	LookAt(GetPosition(), XMLoadFloat3(&target), GetUpDir());
}

void Transform::LookAt(const Transform& target)
{
	LookAt(GetPosition(), target.GetPosition(), GetUpDir());
}

const DirectX::XMMATRIX Transform::GetMatrix()
{
	if (mIsDirty)
	{
		XMMATRIX newMatrix = DirectX::XMMatrixIdentity();
		newMatrix *= XMMatrixRotationRollPitchYawFromVector(XMLoadFloat3(&mAngles));
		newMatrix *= DirectX::XMMatrixScaling(mScale.x, mScale.y, mScale.z);
		newMatrix *= DirectX::XMMatrixTranslation(mPosition.x, mPosition.y, mPosition.z);
		XMStoreFloat4x4(&mMatrix, newMatrix);
		mIsDirty = false;
	}

	return XMLoadFloat4x4(&mMatrix);
}

const DirectX::XMFLOAT4X4 Transform::GetMatrixf()
{
	if (mIsDirty)
	{
		XMMATRIX newMatrix = DirectX::XMMatrixIdentity();
		newMatrix *= XMMatrixRotationRollPitchYawFromVector(XMLoadFloat3(&mAngles));
		newMatrix *= DirectX::XMMatrixScaling(mScale.x, mScale.y, mScale.z);
		newMatrix *= DirectX::XMMatrixTranslation(mPosition.x, mPosition.y, mPosition.z);
		XMStoreFloat4x4(&mMatrix, newMatrix);
		mIsDirty = false;
	}

	return mMatrix;
}

// Move in global space
void Transform::MoveX(float distance)
{
	mPosition.x += distance,
		mIsDirty = true;
}

void Transform::MoveY(float distance)
{
	mPosition.y += distance,
		mIsDirty = true;
}

void Transform::MoveZ(float distance)
{
	mPosition.z += distance,
		mIsDirty = true;
}

// Move in local space
void Transform::MoveRight(float distance)
{
	XMVECTOR vecDistance = XMVectorReplicate(distance);
	XMVECTOR vecRight = GetRightDir();
	XMVECTOR vecPosition = GetPosition();
	XMStoreFloat3(&mPosition, XMVectorMultiplyAdd(vecDistance, vecRight, vecPosition));

	mIsDirty = true;
}

void Transform::MoveUp(float distance)
{
	XMVECTOR vecDistance = XMVectorReplicate(distance);
	XMVECTOR vecUp = GetUpDir();
	XMVECTOR vecPosition = GetPosition();
	XMStoreFloat3(&mPosition, XMVectorMultiplyAdd(vecDistance, vecUp, vecPosition));

	mIsDirty = true;
}

void Transform::MoveForward(float distance)
{
	XMVECTOR vecDistance = XMVectorReplicate(distance);
	XMVECTOR vecForward = GetForwardDir();
	XMVECTOR vecPosition = GetPosition();
	XMStoreFloat3(&mPosition, XMVectorMultiplyAdd(vecDistance, vecForward, vecPosition));

	mIsDirty = true;
}

// Rotation
void Transform::RotateX(float angle)
{
	mAngles.x += angle;
	mIsDirty = true;
}

void Transform::RotateY(float angle)
{
	mAngles.y += angle;
	mIsDirty = true;
}

void Transform::RotateZ(float angle)
{
	mAngles.z += angle;
	mIsDirty = true;
}