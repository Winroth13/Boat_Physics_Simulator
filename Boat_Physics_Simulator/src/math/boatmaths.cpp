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