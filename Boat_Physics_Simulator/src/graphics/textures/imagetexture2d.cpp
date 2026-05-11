#include "core/logger.h"
#include "core/renderer/renderer.h"
#include "graphics/textures/imagetexture2d.h"

#include "stbImage/stb_image.h"
#include "imgui/imgui.h"

ImageTexture2D::ImageTexture2D(const std::string& path)
{
	mPath = path;

	int width, height, channels;
	char* imageData = LoadImageData(path, width, height, channels);

	if (imageData != nullptr)
	{
		Logger::Info("Loaded texture: " + path);
	}
	else
	{
		/* If texture couldn't be loaded, use error texture instead */
		imageData = LoadImageData("resources/fallback.png", width, height, channels);
		Logger::Warn("Failed to load texture from path: " + path + " (using fallback)");
		mPath += " <ERROR>";
	}

	mWidth = width;
	mHeight = height;
	mChannels = channels;

	/* Convert STBI channels to DXGI Texture Format */
	DXGI_FORMAT textureFormat;
	switch (mChannels)
	{
	case STBI_rgb:
	case STBI_rgb_alpha:
		textureFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
		break;

		//case STBI_rgb:
		//	textureFormat = DXGI_FORMAT_R8G8B8_UNORM;
		//	break;

	case STBI_grey:
		Logger::Warn("Grey scale.");
		textureFormat = DXGI_FORMAT_R8_UNORM;
		break;

	default:
		Logger::Warn("Texture has unsupported format: " + std::to_string(mChannels) + ", falling back to RGBA");
		textureFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
		break;
	}

	/* Create Texture */
	D3D11_TEXTURE2D_DESC desc = {};
	desc.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_RENDER_TARGET;
	desc.MipLevels = 0;
	desc.ArraySize = 1;
	desc.Format = textureFormat;
	desc.Width = mWidth;
	desc.Height = mHeight;
	desc.SampleDesc.Count = 1;
	desc.SampleDesc.Quality = 0;
	desc.Usage = D3D11_USAGE_DEFAULT;
	desc.CPUAccessFlags = 0;
	desc.MiscFlags = D3D11_RESOURCE_MISC_GENERATE_MIPS;

	HRESULT hr = Renderer::GetDevice()->CreateTexture2D(&desc, nullptr, &mTexture);

	if (FAILED(hr))
	{
		Logger::Error("DirectX failed to create Texture2D for texture: " + path);
		throw std::runtime_error("");
	}

	UINT rowPitch = mWidth * sizeof(char) * 4;
	UINT imageSize = rowPitch * mHeight;

	Renderer::GetContext()->UpdateSubresource(mTexture, 0, nullptr, imageData, rowPitch, imageSize);

	stbi_image_free(imageData);

	D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Format = textureFormat;
	srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Texture2D.MipLevels = -1;

	hr = Renderer::GetDevice()->CreateShaderResourceView(mTexture, &srvDesc, &mShaderResourceView);

	if (FAILED(hr))
	{
		Logger::Error("DirectX failed to shader resource view for texture: " + path);
		throw std::runtime_error("");
	}

	Renderer::GetContext()->GenerateMips(mShaderResourceView);

	/* Create Render Target View */
	hr = Renderer::GetDevice()->CreateRenderTargetView(mTexture, nullptr, &mRenderTargetView);
	if (FAILED(hr))
	{
		Logger::Error("Failed to create render target view for texture: " + path);
		throw std::runtime_error("");
	}
}

ImageTexture2D::~ImageTexture2D()
{
	mRenderTargetView->Release();
}

void ImageTexture2D::RenderImgui(const uint32_t width, const uint32_t height)
{
	ImGui::Text("%s (%dx%d)", mPath.c_str(), mWidth, mHeight);

	float aspect = (float)mWidth / mHeight;

	ImGui::Image(
		(ImTextureID)(intptr_t)mShaderResourceView,
		ImVec2(static_cast<float>(width), static_cast<float>(height / aspect))
	);
}