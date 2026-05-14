#include "math/pointcloud.h"

#include "core/logger.h"

#include "tinyobjloader/tiny_obj_loader.h"

PointCloud::PointCloud(const std::string& pointCloudPath, float totalMass)
{
	ReadPoints(pointCloudPath);
	mTotalMass = totalMass;
	mPointMass = mTotalMass / mPoints.size();
}

PointCloud::PointCloud(const std::string& pointCloudPath, float pointRadius, float density)
{
	ReadPoints(pointCloudPath);
	float pointVolume = (4.0f / 3.0f) * DirectX::XM_PI * powf(pointRadius, 3);
    float totalVolume = pointVolume * mPoints.size();

	mTotalMass = totalVolume * density;
	mPointMass = pointVolume * density;
}

PointCloud::~PointCloud()
{

}

void PointCloud::ReadPoints(std::string path)
{
	tinyobj::ObjReaderConfig config;
	config.mtl_search_path = "";

	tinyobj::ObjReader reader;

	if (!reader.ParseFromFile(path, config))
	{
		if (!reader.Error().empty())
		{
			Logger::Error("TinyOBJ failed to parse file: " + path + ", reason: " + reader.Error());
			throw std::runtime_error("");
		}
	}

	if (!reader.Warning().empty())
	{
		Logger::Warn("TinyOBJ warning: " + reader.Warning());
	}

	auto& vertices = reader.GetAttrib().vertices;

	for (size_t i = 0; i < vertices.size(); i += 3)
	{
		float x = vertices[i + 0];
		float y = vertices[i + 1];
		float z = vertices[i + 2];

		Point point;
		point.position = { x, y, z };
		mPoints.push_back(point);
	}
}
