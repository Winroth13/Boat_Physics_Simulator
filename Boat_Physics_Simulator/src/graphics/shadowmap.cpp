#include "graphics/shadowmap.h"
#include "core/renderer/renderer.h"
#include "core/logger.h"

#include <iostream>

ShadowMap::ShadowMap()
{
}

ShadowMap::~ShadowMap()
{
	if (mSrv != nullptr)
	{
		mSrv->Release();
	}

	/* Delete all depth stencil views */
	for (size_t i = 0; i < mMaxMaps; ++i)
	{
		mDepthStencilViews[i]->Release();
	}
	delete[] mDepthStencilViews;

	if (mDepthTextureArray != nullptr)
	{
		mDepthTextureArray->Release();
	}
}

bool ShadowMap::Create(size_t maxNumMaps, size_t textureWidth, size_t textureHeight)
{
	mMaxMaps = maxNumMaps;
	mWidth = textureWidth;
	mHeight = textureWidth;

	/* Configure Viewport */
	mViewport.TopLeftX = 0;
	mViewport.TopLeftY = 0;
	mViewport.Width = static_cast<float>(textureWidth);
	mViewport.Height = static_cast<float>(textureHeight);
	mViewport.MinDepth = 0;
	mViewport.MaxDepth = 1;

	/* Create Depth Texture Array */
	D3D11_TEXTURE2D_DESC arrDesc = {};
	arrDesc.Width = static_cast<UINT>(textureWidth);
	arrDesc.Height = static_cast<UINT>(textureHeight);
	arrDesc.MipLevels = 1;
	arrDesc.ArraySize = static_cast<UINT>(mMaxMaps);
	arrDesc.Format = DXGI_FORMAT_R32_TYPELESS;
	arrDesc.SampleDesc.Count = 1;
	arrDesc.SampleDesc.Quality = 0;
	arrDesc.Usage = D3D11_USAGE_DEFAULT;
	arrDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL | D3D11_BIND_SHADER_RESOURCE;
	arrDesc.CPUAccessFlags = 0;
	arrDesc.MiscFlags = 0;

	if (FAILED(Renderer::GetDevice()->CreateTexture2D(&arrDesc, nullptr, &mDepthTextureArray)))
	{
		Logger::Error("Failed to create depth texture array");
		return false;
	}

	/* Create Depth Stencil Views */
	D3D11_DEPTH_STENCIL_VIEW_DESC dsvDesc = {};
	dsvDesc.Format = DXGI_FORMAT_D32_FLOAT;
	dsvDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2DARRAY;
	dsvDesc.Texture2DArray.MipSlice = 0;
	dsvDesc.Texture2DArray.ArraySize = 1;

	mDepthStencilViews = new ID3D11DepthStencilView * [mMaxMaps];
	for (UINT dsvIndex = 0; dsvIndex < mMaxMaps; ++dsvIndex)
	{
		dsvDesc.Texture2DArray.FirstArraySlice = dsvIndex;

		if (FAILED(Renderer::GetDevice()->CreateDepthStencilView(
			mDepthTextureArray,
			&dsvDesc,
			&mDepthStencilViews[dsvIndex]))
			)
		{
			Logger::Error("Failed to create depth stencil view");
			return false;
		}
	}

	/* Set Shader Resource View Desc */
	ZeroMemory(&mSrvDesc, sizeof(mSrvDesc));
	mSrvDesc.Format = DXGI_FORMAT_R32_FLOAT;
	mSrvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2DARRAY;
	mSrvDesc.Texture2DArray.MostDetailedMip = 0;
	mSrvDesc.Texture2DArray.MipLevels = 1;
	mSrvDesc.Texture2DArray.FirstArraySlice = 0;
	mSrvDesc.Texture2DArray.ArraySize = static_cast<UINT>(mMaxMaps);

	return true;
}

ID3D11ShaderResourceView* ShadowMap::GetSRV(size_t numMaps)
{
	if (mSrv != nullptr)
	{
		mSrv->Release();
	}

	mSrvDesc.Texture2DArray.ArraySize = static_cast<UINT>(numMaps);

	if (FAILED(Renderer::GetDevice()->CreateShaderResourceView(mDepthTextureArray, &mSrvDesc, &mSrv)))
	{
		Logger::Error("Failed to create shader resource view");
		throw std::runtime_error("");
	}

	return mSrv;
}