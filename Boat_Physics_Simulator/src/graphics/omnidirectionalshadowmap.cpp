#include "graphics/omnidirectionalshadowmap.h"
#include "core/renderer/renderer.h"
#include "core/logger.h"

OmnidirectionalShadowMap::OmnidirectionalShadowMap()
{
}

OmnidirectionalShadowMap::~OmnidirectionalShadowMap()
{
	if (mRtvArray)
	{
		for (size_t i = 0; i < mMaxMaps * 6; ++i)
		{
			mRtvArray[i]->Release();
		}
		delete[] mRtvArray;
	}

	if (mDsv)
	{
		mDsv->Release();
	}

	if (mSrv)
	{
		mSrv->Release();
	}

	if (mDepthTexture)
	{
		mDepthTexture->Release();
	}

	if (mTexture)
	{
		mTexture->Release();
	}
}

bool OmnidirectionalShadowMap::Create(size_t maxNumMaps, size_t textureWidth, size_t textureHeight)
{
	mMaxMaps = maxNumMaps;
	mWidth = textureWidth;
	mHeight = textureHeight;

	/* Configure Viewport */
	{
		mViewport.TopLeftX = 0;
		mViewport.TopLeftY = 0;
		mViewport.Width = static_cast<float>(textureWidth);
		mViewport.Height = static_cast<float>(textureHeight);
		mViewport.MinDepth = 0;
		mViewport.MaxDepth = 1;
	}

	/* Create Depth Texture */
	{
		D3D11_TEXTURE2D_DESC desc = {};
		desc.Width = static_cast<UINT>(textureWidth);;
		desc.Height = static_cast<UINT>(textureHeight);
		desc.MipLevels = 1;
		desc.ArraySize = 1;
		desc.Format = DXGI_FORMAT_R32_TYPELESS;
		desc.SampleDesc.Count = 1;
		desc.SampleDesc.Quality = 0;
		desc.Usage = D3D11_USAGE_DEFAULT;
		desc.BindFlags = D3D11_BIND_DEPTH_STENCIL;
		desc.CPUAccessFlags = 0;
		desc.MiscFlags = 0;

		HRESULT hr = Renderer::GetDevice()->CreateTexture2D(&desc, nullptr, &mDepthTexture);

		if (FAILED(hr))
		{
			Logger::Error("Failed to create cubemap depth texture");
			return false;
		}
	}

	/* Create Texture */
	{
		D3D11_TEXTURE2D_DESC desc = {};
		desc.Width = static_cast<UINT>(textureWidth);
		desc.Height = static_cast<UINT>(textureHeight);
		desc.MipLevels = 1;
		desc.ArraySize = static_cast<UINT>(mMaxMaps) * 6;
		desc.Format = DXGI_FORMAT_R32_FLOAT;
		desc.SampleDesc.Count = 1;
		desc.SampleDesc.Quality = 0;
		desc.Usage = D3D11_USAGE_DEFAULT;
		desc.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_RENDER_TARGET;
		desc.CPUAccessFlags = 0;
		desc.MiscFlags = D3D11_RESOURCE_MISC_TEXTURECUBE;

		if (FAILED(Renderer::GetDevice()->CreateTexture2D(&desc, nullptr, &mTexture)))
		{
			Logger::Error("Failed to create texture");
			return false;
		}
	}

	/* Depth Depth Stencil View */
	{
		D3D11_DEPTH_STENCIL_VIEW_DESC desc = {};
		desc.Format = DXGI_FORMAT_D32_FLOAT;
		desc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2DARRAY;
		desc.Texture2DArray.MipSlice = 0;
		desc.Texture2DArray.ArraySize = 1;

		if (FAILED(Renderer::GetDevice()->CreateDepthStencilView(mDepthTexture, &desc, &mDsv)))
		{
			Logger::Error("Failed to create depth stencil view");
			return false;
		}
	}

	/* Create Render Target Views */
	{
		mRtvArray = new ID3D11RenderTargetView * [maxNumMaps * 6];

		D3D11_RENDER_TARGET_VIEW_DESC desc = {};
		desc.Format = DXGI_FORMAT_R32_FLOAT;
		desc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2DARRAY;
		desc.Texture2DArray.MipSlice = 0;
		desc.Texture2DArray.ArraySize = 1;

		for (size_t i = 0; i < maxNumMaps * 6; ++i)
		{
			desc.Texture2DArray.FirstArraySlice = static_cast<UINT>(i);

			if (FAILED(Renderer::GetDevice()->CreateRenderTargetView(
				mTexture,
				&desc,
				&mRtvArray[i]))
				)
			{
				Logger::Error("Failed to create render target view");
				return false;
			}
		}
	}

	/* Create SRV Descriptor */
	{
		mSrvDesc = {};
		mSrvDesc.Format = DXGI_FORMAT_R32_FLOAT;
		mSrvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURECUBEARRAY;
		mSrvDesc.TextureCube.MipLevels = 1;
		mSrvDesc.TextureCubeArray.First2DArrayFace = 0;
		mSrvDesc.TextureCubeArray.MipLevels = 1;
		mSrvDesc.TextureCubeArray.MostDetailedMip = 0;
		mSrvDesc.TextureCubeArray.NumCubes = static_cast<UINT>(maxNumMaps);
	}

	return true;
}

ID3D11ShaderResourceView* OmnidirectionalShadowMap::GetSRV(const size_t numMaps)
{
	if (mSrv != nullptr)
	{
		mSrv->Release();
	}

	mSrvDesc.TextureCubeArray.NumCubes = static_cast<UINT>(numMaps);

	HRESULT hr = Renderer::GetDevice()->CreateShaderResourceView(mTexture, &mSrvDesc, &mSrv);

	if (FAILED(hr))
	{
		Logger::Error("Failed to create omnidirectional shadow map shader resource view");
		throw std::runtime_error("");
	}

	return mSrv;
}