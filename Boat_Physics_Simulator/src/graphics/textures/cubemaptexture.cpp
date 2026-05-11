#include "graphics/textures/cubemaptexture.h"
#include "graphics/gbuffers.h"
#include "core/renderer/renderer.h"
#include "core/logger.h"

#include <d3d11.h>

#include "stbImage/stb_image.h"
#include "imgui/imgui.h"

CubemapTexture::CubemapTexture(uint32_t width, uint32_t height)
	:mWidth(width), mHeight(height)
{
	if (!CreateTexture(nullptr))
	{
		throw std::runtime_error("");
	}

	if (!CreateSRV())
	{
		throw std::runtime_error("");
	}

	if (!CreateUAVs())
	{
		throw std::runtime_error("");
	}

	if (!CreateGBuffers())
	{
		throw std::runtime_error("");
	}

	if (!CreateRTVs())
	{
		throw std::runtime_error("");
	}
}

CubemapTexture::CubemapTexture(const std::array<std::string, 6>& paths)
{
	int width, height, channels;
	char** data = new char* [6];

	for (int i = 0; i < 6; ++i)
	{
		char* imageData = LoadImageData(paths[i], width, height, channels);

		if (imageData != nullptr)
		{
			Logger::Info("Loaded texture: " + paths[i]);
		}
		else
		{
			/* If texture couldn't be loaded, use error texture instead */
			imageData = LoadImageData("resources/fallback.png", width, height, channels);
			Logger::Warn("Failed to load texture from path: " + paths[i] + " (using fallback)");
			mPath += " <ERROR>";
		}

		data[i] = imageData;
	}

	mWidth = width;
	mHeight = height;
	mChannels = channels;

	if (!CreateTexture(data))
	{
		throw std::runtime_error("");
	}

	/* Free memory */
	for (int i = 0; i < 6; ++i)
	{
		stbi_image_free(data[i]);
	}
	delete[] data;

	if (!CreateSRV())
	{
		throw std::runtime_error("");
	}
}

CubemapTexture::~CubemapTexture()
{
	/* Delete unordered access views */
	if (mUnorderedAccessViews != nullptr)
	{
		for (size_t i = 0; i < 6; ++i)
		{
			mUnorderedAccessViews[i]->Release();
		}
		delete[] mUnorderedAccessViews;
	}

	/* Delete render target views */
	if (mRenderTargetViews != nullptr)
	{
		for (size_t i = 0; i < 6; ++i)
		{
			mRenderTargetViews[i]->Release();
		}
		delete[] mRenderTargetViews;
	}
}

void CubemapTexture::RenderImgui(const uint32_t width, const uint32_t height)
{
	/*ImGui::Image(
		(ImTextureID)(intptr_t)mShaderResourceView,
		ImVec2(static_cast<float>(width), static_cast<float>(height))
	);*/
}

bool CubemapTexture::CreateSRV()
{
	D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURECUBE;
	srvDesc.TextureCube.MipLevels = 1;

	if (
		FAILED(Renderer::GetDevice()->CreateShaderResourceView(
			mTexture,
			&srvDesc,
			&mShaderResourceView))
		)
	{
		Logger::Error("Failed to create cubemap texture shader resource view");
		return false;
	}

	return true;
}

bool CubemapTexture::CreateTexture(char** data)
{
	D3D11_TEXTURE2D_DESC textureDesc = {};
	textureDesc.Width = mWidth;
	textureDesc.Height = mHeight;
	textureDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	textureDesc.Usage = D3D11_USAGE_DEFAULT;
	textureDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_UNORDERED_ACCESS | D3D11_BIND_RENDER_TARGET;
	textureDesc.CPUAccessFlags = 0;
	textureDesc.MipLevels = 1;
	textureDesc.ArraySize = 6;
	textureDesc.SampleDesc.Count = 1;
	textureDesc.SampleDesc.Quality = 0;
	textureDesc.MiscFlags = D3D11_RESOURCE_MISC_TEXTURECUBE;

	HRESULT hr;
	if (data == nullptr)
	{
		hr = Renderer::GetDevice()->CreateTexture2D(&textureDesc, nullptr, &mTexture);
	}
	else
	{
		D3D11_SUBRESOURCE_DATA texData[6] = {};

		for (int i = 0; i < 6; ++i)
		{
			texData[i].pSysMem = data[i];
			texData[i].SysMemPitch = mWidth * 4;
			texData[i].SysMemSlicePitch = 0;
		}

		hr = Renderer::GetDevice()->CreateTexture2D(&textureDesc, texData, &mTexture);
	}

	if (FAILED(hr))
	{
		Logger::Error("Failed to create cubemap texture");
		return false;
	}

	return true;
}

bool CubemapTexture::CreateUAVs()
{
	/* Configure Viewport */
	mViewport.TopLeftX = 0;
	mViewport.TopLeftY = 0;
	mViewport.Width = static_cast<float>(mWidth);
	mViewport.Height = static_cast<float>(mHeight);
	mViewport.MinDepth = 0;
	mViewport.MaxDepth = 1;

	/* Create render target views */
	mUnorderedAccessViews = new ID3D11UnorderedAccessView * [6];

	D3D11_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
	uavDesc.ViewDimension = D3D11_UAV_DIMENSION_TEXTURE2DARRAY;
	uavDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	uavDesc.Texture2DArray.MipSlice = 0;
	uavDesc.Texture2DArray.ArraySize = 1;

	for (int i = 0; i < 6; ++i)
	{
		uavDesc.Texture2DArray.FirstArraySlice = i;
		if (FAILED(Renderer::GetDevice()->CreateUnorderedAccessView(
			mTexture,
			&uavDesc,
			&mUnorderedAccessViews[i]))
			)
		{
			Logger::Error("Failed to create unordered access view for cubemap texture");
			return false;
		}
	}

	return true;
}

bool CubemapTexture::CreateRTVs()
{
	/* Create render target views */
	mRenderTargetViews = new ID3D11RenderTargetView * [6];

	D3D11_RENDER_TARGET_VIEW_DESC rtvDesc = {};
	rtvDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2DARRAY;
	rtvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	rtvDesc.Texture2DArray.MipSlice = 0;
	rtvDesc.Texture2DArray.ArraySize = 1;

	for (int i = 0; i < 6; ++i)
	{
		rtvDesc.Texture2DArray.FirstArraySlice = i;
		if (FAILED(Renderer::GetDevice()->CreateRenderTargetView(
			mTexture,
			&rtvDesc,
			&mRenderTargetViews[i]))
			)
		{
			Logger::Error("Failed to create render target views for cubemap texture");
			return false;
		}
	}

	return true;
}

bool CubemapTexture::CreateGBuffers()
{
	if (!mGBuffers.Create(mWidth, mHeight))
	{
		Logger::Error("Cubemap texture failed to create gbuffers");
		return false;
	}

	return true;
}