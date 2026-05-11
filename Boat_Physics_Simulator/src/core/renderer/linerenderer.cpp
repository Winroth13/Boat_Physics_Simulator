#include "core/renderer/aabbrenderer.h"
#include "core/renderer/renderer.h"
#include "core/logger.h"

#include "graphics/shaders/vertexshader.h"
#include "graphics/shaders/pixelshader.h"

#include <d3d11.h>

LineRenderer::LineRenderer()
{
}

LineRenderer::~LineRenderer()
{
	if (mInputLayout)
	{
		mInputLayout->Release();
	}

	if (mVertexBuffer)
	{
		mVertexBuffer->Release();
	}
}

bool LineRenderer::Create()
{
	mVertexShader = std::make_unique<VertexShader>("resources/LineVertexShader.cso");
	mPixelShader = std::make_unique<PixelShader>("resources/LinePixelShader.cso");

	D3D11_INPUT_ELEMENT_DESC inputDesc[2] =
	{
		{"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0},
		{"COLOR", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0}
	};

	HRESULT hr = Renderer::GetDevice()->CreateInputLayout(
		inputDesc,
		2,
		mVertexShader->GetByteCode().c_str(),
		mVertexShader->GetByteCode().length(),
		&mInputLayout
	);

	if (FAILED(hr))
	{
		Logger::Error("Failed to create line input layout");
		return false;
	}

	/* Vertex buffer */
	{
		D3D11_BUFFER_DESC vertexBufferDesc = {};
		vertexBufferDesc.ByteWidth = UINT(sizeof(LineVertex) * 2);
		vertexBufferDesc.Usage = D3D11_USAGE_DYNAMIC;
		vertexBufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
		vertexBufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
		vertexBufferDesc.MiscFlags = 0;
		vertexBufferDesc.StructureByteStride = 0;

		LineVertex vertices[2];
		D3D11_SUBRESOURCE_DATA data = {};
		data.pSysMem = vertices;
		data.SysMemPitch = 0;
		data.SysMemSlicePitch = 0;

		hr = Renderer::GetDevice()->CreateBuffer(&vertexBufferDesc, &data, &mVertexBuffer);

		if (FAILED(hr))
		{
			Logger::Error("Failed to create line vertex buffer");
			return false;
		}
	}

	return true;
}

void LineRenderer::Render(ID3D11DeviceContext* ctx, const std::vector<LineData>& data)
{
	if (data.size() == 0)
	{
		return;
	}

	/* Setup Input Assembler */
	ctx->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_LINELIST);
	ctx->IASetInputLayout(mInputLayout);

	/* Bind Shaders	*/
	ctx->VSSetShader(mVertexShader->GetShader(), nullptr, 0);
	ctx->PSSetShader(mPixelShader->GetShader(), nullptr, 0);

	/* Bind Mesh */
	UINT stride = sizeof(LineVertex);
	UINT offset = 0;
	ctx->IASetVertexBuffers(0, 1, &mVertexBuffer, &stride, &offset);

	/* Draw Lines */
	for (auto& line : data)
	{
		/* Update Mesh */
		LineVertex vertices[2];
		vertices[0].position = line.start;
		vertices[1].position = line.end;
		vertices[0].color = line.color;
		vertices[1].color = line.color;

		D3D11_MAPPED_SUBRESOURCE mappedResource;
		HRESULT result = ctx->Map(mVertexBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource);
		if (FAILED(result))
		{
			Logger::Error("Failed to map line vertex buffer");
			throw std::runtime_error("");
		}

		size_t numBytes = 2 * sizeof(LineVertex);
		memcpy_s(mappedResource.pData, numBytes, vertices, numBytes);
		ctx->Unmap(mVertexBuffer, 0);

		ctx->Draw(2, 0);
	}

	/* Unbind shaders */
	ctx->VSSetShader(nullptr, nullptr, 0);
	ctx->PSSetShader(nullptr, nullptr, 0);

	/* Reset topology */
	ctx->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
}