#pragma once

#include "graphics/shaders/vertexshader.h"
#include "graphics/shaders/pixelshader.h"
#include "graphics/shaders/computeshader.h"

#include <d3d11.h>

class AreaCalculator
{
public:
	AreaCalculator();
	~AreaCalculator();

	bool Create();

	float CalculateArea(
		std::shared_ptr<Model> model, 
		DirectX::XMMATRIX transform,
		DirectX::XMFLOAT3 angles
	);

private:
	std::unique_ptr<VertexShader> mVertexShader;
	std::unique_ptr<PixelShader> mPixelShader;
	std::unique_ptr<ComputeShader> mComputeShader;

	ID3D11Texture2D* mTexture;
	ID3D11RenderTargetView* mTextureRTV;
	ID3D11UnorderedAccessView* mTextureUAV;
	ID3D11ShaderResourceView* mTextureSRV;

	ID3D11Buffer* mAreaBuffer;
	ID3D11UnorderedAccessView* mAreaBufferUAV;
	ID3D11Buffer* mAreaStagingBuffer;

	D3D11_VIEWPORT mViewport;
};