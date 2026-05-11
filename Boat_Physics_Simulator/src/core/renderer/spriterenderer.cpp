#include "core/renderer/spriterenderer.h"

#include "graphics/shaders/vertexshader.h"
#include "graphics/shaders/pixelshader.h"
#include "graphics/shaders/geometryshader.h"

#include "core/renderer/renderer.h"
#include "core/logger.h"

#include <d3d11.h>

SpriteRenderer::SpriteRenderer()
{

}

SpriteRenderer::~SpriteRenderer()
{
	if (mInputLayout)
	{
		mInputLayout->Release();
	}
}

bool SpriteRenderer::Create()
{
	mVertexShader = std::make_unique<VertexShader>("resources/SpriteVertexShader.cso");
	mPixelShader = std::make_unique<PixelShader>("resources/SpritePixelShader.cso");
	mGeometryShader = std::make_unique<GeometryShader>("resources/SpriteGeometryShader.cso");

	/* Create Input Layout */
	{
		D3D11_INPUT_ELEMENT_DESC inputDesc[3] =
		{
			{"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0},
			{"SCALE", 0, DXGI_FORMAT_R32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0},
			{"TINT", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 16, D3D11_INPUT_PER_VERTEX_DATA, 0},
		};

		HRESULT hr = Renderer::GetDevice()->CreateInputLayout(
			inputDesc,
			3,
			mVertexShader->GetByteCode().c_str(),
			mVertexShader->GetByteCode().length(),
			&mInputLayout
		);

		if (FAILED(hr))
		{
			Logger::Error("Failed to create input layout");
			return false;
		}
	}

	return true;
}

void SpriteRenderer::Render(
	ID3D11DeviceContext* ctx,
	RenderServer& renderServer,
	std::vector<SpriteData>& data
)
{
	if (data.size() < 1)
	{
		return;
	}

	/* Configure Input Assembler */
	ctx->IASetInputLayout(mInputLayout);
	ctx->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_POINTLIST);

	/* Bind Shaders */
	ctx->VSSetShader(mVertexShader->GetShader(), nullptr, 0);
	ctx->PSSetShader(mPixelShader->GetShader(), nullptr, 0);
	ctx->GSSetShader(mGeometryShader->GetShader(), nullptr, 0);

	/* Depth Stencil State */
	ID3D11DepthStencilState* depthState = nullptr;
	D3D11_DEPTH_STENCIL_DESC dsDesc = {};
	dsDesc.DepthEnable = true;
	dsDesc.DepthFunc = D3D11_COMPARISON_LESS;
	dsDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
	dsDesc.StencilEnable = false;
	Renderer::GetDevice()->CreateDepthStencilState(&dsDesc, &depthState);

	ctx->OMSetDepthStencilState(depthState, 0);

	for (SpriteData& s : data)
	{
		/* Create Vertex Buffer */
		ID3D11Buffer* vertexBuffer = nullptr;

		D3D11_BUFFER_DESC vertexBufferDesc = {};
		vertexBufferDesc.ByteWidth = UINT(sizeof(SpriteBuffer));
		vertexBufferDesc.Usage = D3D11_USAGE_IMMUTABLE;
		vertexBufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
		vertexBufferDesc.CPUAccessFlags = 0;
		vertexBufferDesc.MiscFlags = 0;
		vertexBufferDesc.StructureByteStride = 0;

		SpriteBuffer vertex = {};
		vertex.position = { 0,0,0 };
		vertex.scale = s.scale;
		vertex.tint = s.tint;

		D3D11_SUBRESOURCE_DATA vertexData = {};
		vertexData.pSysMem = &vertex;
		vertexData.SysMemPitch = 0;
		vertexData.SysMemSlicePitch = 0;

		HRESULT hr = Renderer::GetDevice()->CreateBuffer(
			&vertexBufferDesc,
			&vertexData,
			&vertexBuffer
		);

		if (FAILED(hr))
		{
			Logger::Error("Failed to create sprite vertex buffer");
			throw std::runtime_error("");
		}

		/* Bind Vertex Buffer */
		UINT stride = sizeof(Vertex);
		UINT offset = 0;
		ctx->IASetVertexBuffers(0, 1, &vertexBuffer, &stride, &offset);

		/* Bind Texture */
		ID3D11ShaderResourceView* texView = s.texture->GetSRV();
		ctx->PSSetShaderResources(0, 1, &texView);

		renderServer.UpdatePerObject(s.transform);

		/* Draw Sprite */
		ctx->Draw(1, 0);

		vertexBuffer->Release();
	}

	/* Unbind Depth Stencil State */
	ctx->OMSetDepthStencilState(nullptr, 0);
	depthState->Release();

	/* Unbind Texture */
	ID3D11ShaderResourceView* nullSrv = nullptr;
	ctx->PSSetShaderResources(0, 1, &nullSrv);

	/* Unbind Shaders */
	ctx->VSSetShader(nullptr, nullptr, 0);
	ctx->PSSetShader(nullptr, nullptr, 0);
	ctx->GSSetShader(nullptr, nullptr, 0);

	ctx->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
}