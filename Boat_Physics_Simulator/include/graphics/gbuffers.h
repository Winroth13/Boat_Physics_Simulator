#pragma once
#include <vector>
#include <d3d11.h>

enum class GBufferType
{
	POSITION,
	NORMAL,
	COLOR,
	MAX
};

class GBuffers
{
public:
	GBuffers();
	~GBuffers();

	bool Create(uint32_t width, uint32_t height);

	const uint32_t GetWidth() const { return mWidth; }
	const uint32_t GetHeight() const { return mHeight; }

	ID3D11ShaderResourceView* GetSRV(GBufferType type) { return mGBufferResourceViews[static_cast<size_t>(type)]; }
	ID3D11RenderTargetView* GetRTV(GBufferType type) { return mGBufferRenderTargetViews[static_cast<size_t>(type)]; }
	ID3D11DepthStencilView* GetDSV() { return mDepthStencilView; }
    ID3D11ShaderResourceView* GetDepthSRV() { return mDepthStencilSRV; }
	D3D11_VIEWPORT& GetViewport() { return mViewport; }

	std::vector<ID3D11ShaderResourceView*>& GetSRVs() { return mGBufferResourceViews; }
	std::vector<ID3D11RenderTargetView*>& GetRTVs() { return mGBufferRenderTargetViews; }

private:
	std::vector<ID3D11Texture2D*> mGBufferTextures;
	std::vector<ID3D11ShaderResourceView*> mGBufferResourceViews;
	std::vector<ID3D11RenderTargetView*> mGBufferRenderTargetViews;

	ID3D11Texture2D* mDepthStencilTexture = nullptr;
	ID3D11DepthStencilView* mDepthStencilView = nullptr;
    ID3D11ShaderResourceView* mDepthStencilSRV = nullptr;

	D3D11_VIEWPORT mViewport = {};
	uint32_t mWidth;
	uint32_t mHeight;
};