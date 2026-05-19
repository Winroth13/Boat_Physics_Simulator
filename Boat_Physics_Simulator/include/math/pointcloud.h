#pragma once

#include "math/aabb.h"

#include <string>
#include <DirectXMath.h>
#include <DirectXCollision.h>
#include <vector>

class PointCloud
{
public:
	struct Point
	{
		DirectX::XMFLOAT3 position = { 0,0,0 };
	};

	PointCloud() {};
	PointCloud(const std::string& pointCloudPath, float totalMass);
	PointCloud(const std::string& pointCloudPath, float pointRadius, float density);
	~PointCloud();

	void AddPoint(Point point);

	const std::vector<Point>& GetPoints() const { return mPoints; }

	float GetTotalMass() const { return mPointMass * mPoints.size(); }
	float GetPointMass() const { return mPointMass; }
	float GetPointRadius() const { return mPointRadius; }
	float GetPointVolume() const { return (4.0f / 3.0f) * DirectX::XM_PI * powf(mPointRadius, 3); }
	float GetVolume() const { return mPoints.size() * GetPointVolume() * (6.0f / DirectX::XM_PI); }

	float GetWidth()  const { return mBounds.mMax.x - mBounds.mMin.x; }
	float GetHeight() const { return mBounds.mMax.y - mBounds.mMin.y; }
	float GetLength() const { return mBounds.mMax.z - mBounds.mMin.z; }

	void SetPointRadius(float radius) { mPointRadius = radius; }
	void SetPointMass(float mass) { mPointMass = mass; }

private:
	void ReadPoints(std::string path);

	std::vector<Point> mPoints;

	float mPointMass = 0.0f;
	float mPointRadius = 0.0f;

    AABB mBounds;
};