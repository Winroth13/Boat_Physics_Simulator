#include "graphics/gbuffers.h"
#include "core/logger.h"
#include "core/renderer/renderer.h"

#include <string>

GBuffers::GBuffers()
{
}

GBuffers::~GBuffers()
{
	for (size_t gBufferIndex = 0; gBufferIndex < mGBufferTextures.size(); ++gBufferIndex)
	{
		auto& texture = mGBufferTextures[gBufferIndex];
		auto& srv = mGBufferResourceViews[gBufferIndex];
		auto& rtv = mGBufferRenderTargetViews[gBufferIndex];
		if (texture)
			texture->Release();
		if (srv)
			srv->Release();
		if (rtv)
			rtv->Release();
	}

	if (mDepthStencilView != nullptr)
		mDepthStencilView->Release();

    if (mDepthStencilSRV != nullptr)
        mDepthStencilSRV->Release();

	if (mDepthStencilTexture != nullptr)
		mDepthStencilTexture->Release();
}

bool GBuffers::Create(uint32_t width, uint32_t height)
{
	mWidth = width;
	mHeight = height;

	/* Viewport */
	{
		mViewport.TopLeftX = 0;
		mViewport.TopLeftY = 0;
		mViewport.Width = static_cast<float>(width);
		mViewport.Height = static_cast<float>(height);
		mViewport.MinDepth = 0;
		mViewport.MaxDepth = 1;
	}

	/* Buffers */
	{
		D3D11_TEXTURE2D_DESC textureDesc = {};
		textureDesc.Width = static_cast<UINT>(width);
		textureDesc.Height = static_cast<UINT>(height);
		textureDesc.MipLevels = 1;
		textureDesc.ArraySize = 1;
		textureDesc.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
		textureDesc.SampleDesc.Count = 1;
		textureDesc.SampleDesc.Quality = 0;
		textureDesc.Usage = D3D11_USAGE_DEFAULT;
		textureDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_RENDER_TARGET;
		textureDesc.CPUAccessFlags = 0;
		textureDesc.MiscFlags = 0;

		for (size_t gBufferIndex = 0; gBufferIndex < static_cast<size_t>(GBufferType::MAX); ++gBufferIndex)
		{
			mGBufferTextures.push_back(nullptr);
			mGBufferResourceViews.push_back(nullptr);
			mGBufferRenderTargetViews.push_back(nullptr);

			auto& texture = mGBufferTextures[gBufferIndex];
			auto& srv = mGBufferResourceViews[gBufferIndex];
			auto& rtv = mGBufferRenderTargetViews[gBufferIndex];

			if (FAILED(Renderer::GetDevice()->CreateTexture2D(&textureDesc, nullptr, &texture)))
			{
				Logger::Error("Failed to create gbuffer texture: " + std::to_string(gBufferIndex));
				return false;
			}

			if (FAILED(Renderer::GetDevice()->CreateShaderResourceView(texture, nullptr, &srv)))
			{
				Logger::Error("Failed to create gbuffer shader resource view: " + std::to_string(gBufferIndex));
				return false;
			}

			if (FAILED(Renderer::GetDevice()->CreateRenderTargetView(texture, nullptr, &rtv)))
			{
				Logger::Error("Failed to create gbuffer render target view: " + std::to_string(gBufferIndex));
				return false;
			}
		}
	}

	/* Depth Stencil */
	{
		D3D11_TEXTURE2D_DESC textureDesc = {};
		textureDesc.Width = static_cast<UINT>(width);
		textureDesc.Height = static_cast<UINT>(height);
		textureDesc.MipLevels = 1;
		textureDesc.ArraySize = 1;
		textureDesc.Format = DXGI_FORMAT_R24G8_TYPELESS;
		textureDesc.SampleDesc.Count = 1;
		textureDesc.SampleDesc.Quality = 0;
		textureDesc.Usage = D3D11_USAGE_DEFAULT;
		textureDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL | D3D11_BIND_SHADER_RESOURCE;
		textureDesc.CPUAccessFlags = 0;
		textureDesc.MiscFlags = 0;

		if (FAILED(Renderer::GetDevice()->CreateTexture2D(&textureDesc, nullptr, &mDepthStencilTexture)))
		{
			Logger::Error("Failed to create depth stencil texture");
			return false;
		}

        D3D11_DEPTH_STENCIL_VIEW_DESC dsvDesc = {};
        dsvDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
        dsvDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;

		if (FAILED(Renderer::GetDevice()->CreateDepthStencilView(mDepthStencilTexture, &dsvDesc, &mDepthStencilView)))
		{
			Logger::Error("Failed to create depth stencil view");
			return false;
		}
	}

	/* Depth Stencil SRV */
	{
        D3D11_SHADER_RESOURCE_VIEW_DESC desc = {};
        desc.Format = DXGI_FORMAT_R24_UNORM_X8_TYPELESS;
        desc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
        desc.Texture2D.MipLevels = 1;
        desc.Texture2D.MostDetailedMip = 0;

		if (FAILED(Renderer::GetDevice()->CreateShaderResourceView(mDepthStencilTexture, &desc, &mDepthStencilSRV)))
		{
            Logger::Error("Failed to create depth stencil srv");
            return false;
		}
	}

	return true;
}