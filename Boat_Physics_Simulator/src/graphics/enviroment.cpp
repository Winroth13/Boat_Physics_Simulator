#include "graphics/enviroment.h"

Enviroment::Enviroment()
{
}

Enviroment::~Enviroment()
{
}

void Enviroment::SetAmbientColor(float r, float g, float b)
{
	mAmbientColor.x = r;
	mAmbientColor.y = g;
	mAmbientColor.z = b;
}

const DirectX::XMFLOAT3 Enviroment::GetAmbientColor()
{
	return mAmbientColor;
}

DirectX::XMFLOAT3& Enviroment::GetAmbientColorRef()
{
	return mAmbientColor;
}

const bool Enviroment::IsUsingBlinnPhong()
{
	return mUseBlinnPhong;
}

bool& Enviroment::GetUseBlinnPhong()
{
	return mUseBlinnPhong;
}
