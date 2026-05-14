#pragma once
#include "math/pointcloud.h"
#include "math/transform.h"

float CalculateWaterViscosity(float waterTemperature, float waterViscosityFactor);

float CalculateReynolds(
    float waterDensity,
    float boatLength,
    float boatVelocity,
    float viscosity
);

/* Empirical calculation of Cf for boat typical shape */
float CalculateCf(float reynoldsNumber);

float CalculateWaterDragForce(
    float waterDensity,
    float areaUnderWater,
    float CH,
    float boatVelocity
);

float CalculateMomentOfInertia(float length, float width, float mass);

DirectX::XMFLOAT3 CalculateCenterOfMass(std::vector<PointCloud>& pointClouds);

void SplitPointCloud(
    const PointCloud& inPointCloud,
    DirectX::XMMATRIX transform,
    float height,
    PointCloud& outAbove,
    PointCloud& outBelow
);

float CalculateFrontArea(PointCloud& pointCloud);