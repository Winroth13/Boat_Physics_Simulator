#pragma once

#include <string>
#include <DirectXMath.h>
#include <vector>

class PointCloud
{
public:
	struct Point
	{
		DirectX::XMFLOAT3 position = { 0,0,0 };
	};

	PointCloud(const std::string& pointCloudPath, float totalMass);
	PointCloud(const std::string& pointCloudPath, float pointRadius, float density);
	~PointCloud();

	float GetTotalMass() const { return mTotalMass; }
	float GetPointMass() const { return mPointMass; }

	std::vector<Point>& GetPoints() { return mPoints; }

private:
	void ReadPoints(std::string path);

	float mTotalMass = 0.0f;
	float mPointMass = 0.0f;
	std::vector<Point> mPoints;
};