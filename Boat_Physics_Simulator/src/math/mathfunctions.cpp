#include "math/mathfunctions.h"
#include <iostream>
#include <sstream>

DirectX::XMFLOAT3 DirectionToAngles(DirectX::XMFLOAT3 direction)
{
	return DirectX::XMFLOAT3(
		atan2f(-direction.y, sqrtf(direction.x * direction.x + direction.z * direction.z)),
		atan2f(direction.x, direction.z),
		0
	);
}

std::string Float3ToString(const DirectX::XMFLOAT3& float3)
{
	std::stringstream stream;
	stream << "(" << float3.x << ", " << float3.y << ", " << float3.z << ")";
	return stream.str();
}

std::string VectorToString(const DirectX::XMVECTOR vector)
{
	DirectX::XMFLOAT3 float3;
	DirectX::XMStoreFloat3(&float3, vector);

	return Float3ToString(float3);
}