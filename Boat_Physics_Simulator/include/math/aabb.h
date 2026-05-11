#pragma once
#include <DirectXMath.h>
#include <DirectXCollision.h>

struct AABB
{
	AABB();
	AABB(const DirectX::BoundingBox& boundingBox);

	DirectX::XMFLOAT3 mMin = { 0, 0, 0 };
	DirectX::XMFLOAT3 mMax = { 0, 0, 0 };

	void Expand(const DirectX::XMFLOAT3& point);

	AABB Transform(const DirectX::XMMATRIX matrix);

	DirectX::BoundingBox ToBoundingBox() const;
};