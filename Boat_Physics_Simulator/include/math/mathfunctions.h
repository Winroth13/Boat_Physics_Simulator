#pragma once
#include <DirectXMath.h>
#include <string>

DirectX::XMFLOAT3 DirectionToAngles(DirectX::XMFLOAT3 direction);

std::string Float3ToString(const DirectX::XMFLOAT3& float3);

std::string VectorToString(const DirectX::XMVECTOR float3);

