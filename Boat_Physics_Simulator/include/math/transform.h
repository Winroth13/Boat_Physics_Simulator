#pragma once
#include <DirectXMath.h>

class Transform
{
public:
	Transform() {};
	~Transform() {};

	// Get/Set tranmsform world position
	DirectX::XMVECTOR GetPosition() const;
	DirectX::XMFLOAT3 GetPosition3f();
	void SetPosition(const float x, const float y, const float z);
	void SetPosition(const DirectX::XMFLOAT3& position);
	void SetPosition(const DirectX::XMVECTOR position);

	void Translate(const float x, const float y, const float z);
	void Translate(const DirectX::XMFLOAT3 position);
	void Translate(const DirectX::XMVECTOR position);

	// Get/Set transform world angles
	DirectX::XMVECTOR GetAngles() const;
	DirectX::XMFLOAT3 GetAngles3f() const;
	void SetAngles(const float pitch, const float yaw, const float roll);
	void SetAngles(const DirectX::XMFLOAT3& angles);
	void SetAngles(const DirectX::XMVECTOR angles);

	void SetPitch(const float angle);
	void SetYaw(const float angle);
	void SetRoll(const float angle);

	void Rotate(const float pitch, const float yaw, const float roll);
	void Rotate(const DirectX::XMFLOAT3 angles);
	void Rotate(const DirectX::XMVECTOR angles);

	// Get/Set transform world scale
	DirectX::XMVECTOR GetScale() const;
	DirectX::XMFLOAT3 GetScale3f() const;
	void SetScale(const float x, const float y, const float z);
	void SetScale(const DirectX::XMFLOAT3& scale);
	void SetScale(const DirectX::XMVECTOR scale);

	void Scale(const float x, const float y, const float z);
	void Scale(const DirectX::XMFLOAT3 scale);
	void Scale(const DirectX::XMVECTOR scale);

	// Get transform basis vectors
	DirectX::XMVECTOR GetRightDir() const;
	DirectX::XMFLOAT3 GetRightDir3f();
	DirectX::XMVECTOR GetUpDir() const;
	DirectX::XMFLOAT3 GetUpDir3f();
	DirectX::XMVECTOR GetForwardDir() const;
	DirectX::XMFLOAT3 GetForwardDir3f();

	// Define transform space via LookAt parameters
	void LookAt(
		DirectX::FXMVECTOR position,
		DirectX::FXMVECTOR target,
		DirectX::FXMVECTOR up
	);
	void LookAt(
		const DirectX::XMFLOAT3& position,
		const DirectX::XMFLOAT3& target,
		const DirectX::XMFLOAT3& up
	);

	void LookAt(DirectX::FXMVECTOR target);
	void LookAt(const DirectX::XMFLOAT3& target);

	void LookAt(const Transform& target);

	const DirectX::XMMATRIX GetMatrix();
	const DirectX::XMFLOAT4X4 GetMatrixf();

	// Move in global space
	void MoveX(float distance);
	void MoveY(float distance);
	void MoveZ(float distance);

	// Move in local space
	void MoveRight(float distance);
	void MoveUp(float distance);
	void MoveForward(float distance);

	// Rotation
	void RotateX(float angle);
	void RotateY(float angle);
	void RotateZ(float angle);

	static DirectX::XMFLOAT3 QuaternionToEulerAngles(const DirectX::XMFLOAT4& quaternion);

private:
	DirectX::XMFLOAT3 mPosition = { 0.0f, 0.0f, 0.0f };
	DirectX::XMFLOAT3 mAngles = { 0, 0, 0 };
	DirectX::XMFLOAT3 mScale = { 1, 1, 1 };
	DirectX::XMFLOAT4X4 mMatrix = {
		1.0f, 0.0f, 0.0f, 0.0f,
		0.0f, 1.0f, 0.0f, 0.0f,
		0.0f, 0.0f, 1.0f, 0.0f,
		0.0f, 0.0f, 0.0f, 1.0f
	};
	bool mIsDirty = false;

	DirectX::XMFLOAT3 AnglesFromMatrix(DirectX::XMFLOAT4X4& matrix);
};