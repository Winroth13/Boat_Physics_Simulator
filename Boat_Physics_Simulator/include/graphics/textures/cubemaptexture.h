#pragma once
#include "graphics/textures/texture2d.h"
#include "graphics/gbuffers.h"
#include <string>

class CubemapTexture : public Texture2D
{
public:
	CubemapTexture(uint32_t width, uint32_t height);
	CubemapTexture(const std::array<std::string, 6>& path);
	~CubemapTexture() override;

	void RenderImgui(const uint32_t width, const uint32_t height) override;

	uint32_t GetWidth() override { return mWidth; }
	uint32_t GetHeight() override { return mHeight; }
	inline const std::string& GetPath() const { return mPath; }

	ID3D11UnorderedAccessView* GetUAV(size_t i) { return mUnorderedAccessViews[i]; }
    ID3D11RenderTargetView* GetRTV(size_t i) { return mRenderTargetViews[i]; }
	GBuffers& GetGBuffers() { return mGBuffers; }

protected:
	bool CreateSRV();
	bool CreateTexture(char** data);
	bool CreateUAVs();
    bool CreateRTVs();
	bool CreateGBuffers();

private:
	std::string mPath;
	uint32_t mWidth = 0;
	uint32_t mHeight = 0;
	uint32_t mChannels = 0;

	ID3D11UnorderedAccessView** mUnorderedAccessViews = nullptr;
    ID3D11RenderTargetView** mRenderTargetViews = nullptr;
	D3D11_VIEWPORT mViewport;
	GBuffers mGBuffers;
};