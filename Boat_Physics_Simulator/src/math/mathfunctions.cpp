#include "math/mathfunctions.h"
#include <iostream>
#include <sstream>

using namespace DirectX;

XMFLOAT3 DirectionToAngles(XMFLOAT3 direction)
{
	return XMFLOAT3(
		atan2f(-direction.y, sqrtf(direction.x * direction.x + direction.z * direction.z)),
		atan2f(direction.x, direction.z),
		0
	);
}

std::string Float3ToString(const XMFLOAT3& float3)
{
	std::stringstream stream;
	stream << "(" << float3.x << ", " << float3.y << ", " << float3.z << ")";
	return stream.str();
}

std::string VectorToString(const XMVECTOR vector)
{
	XMFLOAT3 float3;
	XMStoreFloat3(&float3, vector);

	return Float3ToString(float3);
}

XMFLOAT3 QuaternionToEulerAngles(const XMFLOAT4& quaternion)
{
	XMMATRIX matrix = XMMatrixRotationQuaternion(XMLoadFloat4(&quaternion));
	XMFLOAT4X4 matrixf;
	XMStoreFloat4x4(&matrixf, matrix);

	float pitch = asinf(-matrixf._32);
	float yaw = atan2f(matrixf._31, matrixf._33);
	float roll = atan2f(matrixf._12, matrixf._22);

	//float roll;
	//if (fabsf(pitch) != XM_PIDIV2) // Prevent Gimbal Lock?
	//	roll = atan2f(matrixf._12, matrixf._22);
	//else
	//	roll = 0;

	return XMFLOAT3(pitch, yaw, roll);
}

XMFLOAT3 AnglesFromMatrix(XMMATRIX matrix)
{
	XMVECTOR scale, rotation, translation;
	XMMatrixDecompose(&scale, &rotation, &translation, matrix);
	XMFLOAT4 quaternion;
	XMStoreFloat4(&quaternion, rotation);
	return QuaternionToEulerAngles(quaternion);
}