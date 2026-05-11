#pragma once

#include <array>
#include <DirectXMath.h>

class Enviroment
{
public:
	Enviroment();
	~Enviroment();

	void SetAmbientColor(float r, float g, float b);

	const DirectX::XMFLOAT3 GetAmbientColor();
	DirectX::XMFLOAT3& GetAmbientColorRef();

	const bool IsUsingBlinnPhong();
	bool& GetUseBlinnPhong();

private:
	DirectX::XMFLOAT3 mAmbientColor = { 0.1f, 0.1f, 0.1f };
	bool mUseBlinnPhong = true;
};