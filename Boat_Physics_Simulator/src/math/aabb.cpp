#include "math/aabb.h"

#include <algorithm>

using namespace DirectX;

AABB::AABB()
{
}

AABB::AABB(const DirectX::XMFLOAT3& point)
{
	mMin = point;
	mMax = point;
}

AABB::AABB(const DirectX::BoundingBox& boundingBox)
{
	mMin.x = boundingBox.Center.x - boundingBox.Extents.x;
	mMin.y = boundingBox.Center.y - boundingBox.Extents.y;
	mMin.z = boundingBox.Center.z - boundingBox.Extents.z;

	mMax.x = boundingBox.Center.x + boundingBox.Extents.x;
	mMax.y = boundingBox.Center.y + boundingBox.Extents.y;
	mMax.z = boundingBox.Center.z + boundingBox.Extents.z;
}

void AABB::Expand(const XMFLOAT3& point)
{
	XMVECTOR min = XMLoadFloat3(&mMin);
	XMVECTOR max = XMLoadFloat3(&mMax);
	XMVECTOR p = XMLoadFloat3(&point);

	min = XMVectorMin(min, p);
	max = XMVectorMax(max, p);

	XMStoreFloat3(&mMin, min);
	XMStoreFloat3(&mMax, max);
}

AABB AABB::Transform(const XMMATRIX matrix)
{
	XMVECTOR corners[8] =
	{
		{mMin.x, mMin.y, mMin.z, 1.0f},
		{mMin.x, mMax.y, mMin.z, 1.0f},
		{mMin.x, mMin.y, mMax.z, 1.0f},
		{mMin.x, mMax.y, mMax.z, 1.0f},
		{mMax.x, mMin.y, mMin.z, 1.0f},
		{mMax.x, mMax.y, mMin.z, 1.0f},
		{mMax.x, mMin.y, mMax.z, 1.0f},
		{mMax.x, mMax.y, mMax.z, 1.0f}
	};

	for (int i = 0; i < 8; ++i)
	{
		corners[i] = XMVector4Transform(corners[i], matrix);
	}

	XMVECTOR newMin = corners[0];
	XMVECTOR newMax = newMin;

	for (int i = 1; i < 8; ++i)
	{
		newMin = XMVectorMin(newMin, corners[i]);
		newMax = XMVectorMax(newMax, corners[i]);
	}

	AABB result = {};
	XMStoreFloat3(&result.mMin, newMin);
	XMStoreFloat3(&result.mMax, newMax);

	return result;
}

BoundingBox AABB::ToBoundingBox() const
{
	BoundingBox boundingBox = {};

	XMVECTOR min = XMLoadFloat3(&mMin);
	XMVECTOR max = XMLoadFloat3(&mMax);

	BoundingBox::CreateFromPoints(boundingBox, min, max);

	return boundingBox;
}