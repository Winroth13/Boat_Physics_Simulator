#pragma once
#include <vector>
#include <d3d11.h>

class ShadowMap
{
public:
	ShadowMap();
	~ShadowMap();

	bool Create(size_t maxNumMaps, size_t textureWidth, size_t textureHeight);

	ID3D11ShaderResourceView* GetSRV(size_t numMaps);
	ID3D11DepthStencilView* GetDSV(size_t i) { return mDepthStencilViews[i]; }
	D3D11_VIEWPORT& GetViewport() { return mViewport; }

	const size_t GetWidth() const { return mWidth; }
	const size_t GetHeight() const { return mHeight; }

private:
	size_t mMaxMaps = 0;

	ID3D11Texture2D* mDepthTextureArray = nullptr;
	ID3D11DepthStencilView** mDepthStencilViews;

	ID3D11ShaderResourceView* mSrv = nullptr;
	D3D11_SHADER_RESOURCE_VIEW_DESC mSrvDesc = {};
	D3D11_VIEWPORT mViewport = {};

	size_t mWidth = 0;
	size_t mHeight = 0;
};