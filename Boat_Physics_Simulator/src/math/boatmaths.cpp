#include "math/boatmaths.h"

#include <cmath>

float CalculateWaterViscosity(float waterTemperature, float waterViscosityFactor)
{
    float exponent = 570.6f / (waterTemperature - 140);
    return 2.41e-5f * expf(exponent) * waterViscosityFactor;
}

float CalculateReynolds(
    float waterDensity,
    float boatLength,
    float boatVelocity,
    float viscosity
)
{
    return (waterDensity * boatLength * boatVelocity) / viscosity;
}

float CalculateCf(float reynoldsNumber)
{
    return 3.0f / (40.0f * powf(log10f(reynoldsNumber) - 2.0f, 2.0f));
}

float CalculateWaterDragForce(
    float waterDensity,
    float areaUnderWater,
    float CH,
    float boatVelocity
)
{
    return 0.5f * waterDensity * areaUnderWater * CH * powf(boatVelocity, 2);
}

float CalculateMomentOfInertia(float length, float width, float mass)
{
    float I = mass / 48.0f;
    I *= (4 * powf(length, 2) + 3 * powf(width, 2));
    return I;
}

DirectX::XMFLOAT3 CalculateCenterOfMass(std::vector<PointCloud>& pointClouds)
{
    float totalMass = 0;
    DirectX::XMFLOAT3 totalPoint = { 0,0,0 };

    for (auto& pointCloud : pointClouds)
    {
        auto& points = pointCloud.GetPoints();

        DirectX::XMFLOAT3 sum = { 0.0f, 0.0f, 0.0f };
        for (auto& point : points)
        {
            sum.x += point.position.x;
            sum.y += point.position.y;
			sum.z += point.position.z;
        }
        sum.x /= points.size();
        sum.y /= points.size();
        sum.z /= points.size();

        totalMass += pointCloud.GetTotalMass();
        totalPoint.x += sum.x * pointCloud.GetTotalMass();
        totalPoint.y += sum.x * pointCloud.GetTotalMass();
        totalPoint.z += sum.x * pointCloud.GetTotalMass();
    }

    DirectX::XMFLOAT3 center = { 0,0,0 };
    center.x = totalPoint.x / totalMass;
    center.y = totalPoint.y / totalMass;
    center.z = totalPoint.z / totalMass;

    return center;
}
