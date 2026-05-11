#pragma once
#include <d3d11.h>

class OmnidirectionalShadowMap
{
public:
	OmnidirectionalShadowMap();
	~OmnidirectionalShadowMap();

	bool Create(size_t maxNumMaps, size_t textureWidth, size_t textureHeight);
	ID3D11ShaderResourceView* GetSRV(const size_t numMaps);
	ID3D11DepthStencilView* GetDSV() { return mDsv; }
	ID3D11RenderTargetView* GetRTV(const size_t i, const size_t face) { return mRtvArray[(i * 6) + face]; }
	D3D11_VIEWPORT& GetViewport() { return mViewport; }

	const size_t GetWidth() const { return mWidth; }
	const size_t GetHeight() const { return mHeight; }

private:
	size_t mMaxMaps = 0;

	ID3D11Texture2D* mDepthTexture = nullptr;
	ID3D11DepthStencilView* mDsv = nullptr;

	ID3D11Texture2D* mTexture = nullptr;
	ID3D11ShaderResourceView* mSrv = nullptr;
	ID3D11RenderTargetView** mRtvArray = nullptr;

	D3D11_SHADER_RESOURCE_VIEW_DESC mSrvDesc = {};
	D3D11_VIEWPORT mViewport = {};
	size_t mWidth = 0;
	size_t mHeight = 0;
};