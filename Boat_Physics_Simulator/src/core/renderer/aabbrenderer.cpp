#include "core/renderer/aabbrenderer.h"
#include "core/renderer/renderer.h"
#include "core/logger.h"

#include "graphics/shaders/vertexshader.h"
#include "graphics/shaders/pixelshader.h"

#include <d3d11.h>

constexpr int NUM_VERTS = 8;
constexpr int NUM_INDICIES = 24;
constexpr float VERT_DIST = 1.0f;

AABBRenderer::AABBRenderer()
{
}

AABBRenderer::~AABBRenderer()
{
	if (mInputLayout)
	{
		mInputLayout->Release();
	}

	if (mVertexBuffer)
	{
		mVertexBuffer->Release();
	}

	if (mIndexBuffer)
	{
		mIndexBuffer->Release();
	}

	if (mCBuffer)
	{
		mCBuffer->Release();
	}
}

bool AABBRenderer::Create()
{
	mVertexShader = std::make_unique<VertexShader>("resources/AABBVertexShader.cso");
	mPixelShader = std::make_unique<PixelShader>("resources/AABBPixelShader.cso");

	D3D11_INPUT_ELEMENT_DESC inputDesc[1] =
	{
		{"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0}
	};

	HRESULT hr = Renderer::GetDevice()->CreateInputLayout(
		inputDesc,
		1,
		mVertexShader->GetByteCode().c_str(),
		mVertexShader->GetByteCode().length(),
		&mInputLayout
	);

	if (FAILED(hr))
	{
		Logger::Error("Failed to create AABB input layout");
		return false;
	}

	/* Vertex buffer */
	{
		D3D11_BUFFER_DESC vertexBufferDesc = {};
		vertexBufferDesc.ByteWidth = UINT(sizeof(AABBVertex) * NUM_VERTS);
		vertexBufferDesc.Usage = D3D11_USAGE_DYNAMIC;
		vertexBufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
		vertexBufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
		vertexBufferDesc.MiscFlags = 0;
		vertexBufferDesc.StructureByteStride = 0;

		DirectX::XMFLOAT3 corners[NUM_VERTS] = {};
		AABBVertex vertices[NUM_VERTS];

		for (int i = 0; i < 4; ++i)
		{
			if (i < 2)
			{
				corners[i * 2].x = VERT_DIST;
				corners[i * 2 + 1].x = VERT_DIST;
			}
			else
			{
				corners[i * 2].x = -VERT_DIST;
				corners[i * 2 + 1].x = -VERT_DIST;
			}

			if (i < 2)
			{
				corners[i].y = VERT_DIST;
				corners[i + 4].y = VERT_DIST;
			}
			else
			{
				corners[i].y = -VERT_DIST;
				corners[i + 4].y = -VERT_DIST;
			}

			if (i % 2 == 0)
			{
				corners[i].z = VERT_DIST;
				corners[i + 4].z = VERT_DIST;
			}
			else
			{
				corners[i].z = -VERT_DIST;
				corners[i + 4].z = -VERT_DIST;
			}
		}

		for (int i = 0; i < NUM_VERTS; ++i)
		{
			vertices[i].position = corners[i];
		}

		D3D11_SUBRESOURCE_DATA data = {};
		data.pSysMem = vertices;
		data.SysMemPitch = 0;
		data.SysMemSlicePitch = 0;

		hr = Renderer::GetDevice()->CreateBuffer(&vertexBufferDesc, &data, &mVertexBuffer);

		if (FAILED(hr))
		{
			Logger::Error("Failed to create AABB vertex buffer");
			return false;
		}
	}

	/* Index Buffer */
	{
		D3D11_BUFFER_DESC indexbufferDesc;
		indexbufferDesc.ByteWidth = UINT(sizeof(uint32_t) * NUM_INDICIES);
		indexbufferDesc.Usage = D3D11_USAGE_IMMUTABLE;
		indexbufferDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;
		indexbufferDesc.CPUAccessFlags = 0;
		indexbufferDesc.MiscFlags = 0;

		constexpr uint32_t indices[] = {
			// Back Side
			0, 2,  0, 4,  2, 6,  4, 6,
			// Front Side
			1, 3,  1, 5,  3, 7,  5, 7,
			// Right Side
			0, 1,  2, 3,
			// Left Side
			4, 5,  6, 7
		};

		D3D11_SUBRESOURCE_DATA indexbufferData;
		indexbufferData.pSysMem = indices;
		indexbufferData.SysMemPitch = 0;
		indexbufferData.SysMemSlicePitch = 0;

		hr = Renderer::GetDevice()->CreateBuffer(&indexbufferDesc, &indexbufferData, &mIndexBuffer);

		if (FAILED(hr))
		{
			Logger::Error("Failed to create AABB index buffer");
			return false;
		}
	}

	/* Constant Buffer */
	{
		AABBBuffer buffer = {};

		D3D11_BUFFER_DESC cbufferDesc;
		cbufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
		cbufferDesc.Usage = D3D11_USAGE_DEFAULT;
		cbufferDesc.CPUAccessFlags = 0;
		cbufferDesc.MiscFlags = 0;
		cbufferDesc.ByteWidth = sizeof(buffer);
		cbufferDesc.StructureByteStride = 0;

		D3D11_SUBRESOURCE_DATA data = {};
		data.pSysMem = &buffer;

		hr = Renderer::GetDevice()->CreateBuffer(&cbufferDesc, &data, &mCBuffer);

		if (FAILED(hr))
		{
			Logger::Error("Failed to create AABB constant buffer");
			return false;
		}
	}

	return true;
}

void AABBRenderer::Render(ID3D11DeviceContext* ctx, const std::vector<AABBData>& data)
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
	UINT stride = sizeof(AABBVertex);
	UINT offset = 0;

	ctx->VSSetConstantBuffers(0, 1, &mCBuffer);
	ctx->IASetVertexBuffers(0, 1, &mVertexBuffer, &stride, &offset);
	ctx->IASetIndexBuffer(mIndexBuffer, DXGI_FORMAT_R32_UINT, 0);

	for (auto& aabb : data)
	{
		/* Update AABB Buffer */
		DirectX::XMVECTOR origin = DirectX::XMLoadFloat3(&aabb.origin);
		DirectX::XMVECTOR scale = DirectX::XMLoadFloat3(&aabb.size);

		DirectX::XMMATRIX transform = DirectX::XMMatrixMultiply(
			DirectX::XMMatrixScalingFromVector(scale),
			DirectX::XMMatrixTranslationFromVector(origin)
		);

		AABBBuffer aabbBuffer = {};
		aabbBuffer.transform = transform;
		aabbBuffer.color = aabb.color;
		ctx->UpdateSubresource(mCBuffer, 0, NULL, &aabbBuffer, 0, 0);

		ctx->DrawIndexed(NUM_INDICIES, 0, 0);
	}

	/* Unbind shaders */
	ctx->VSSetShader(nullptr, nullptr, 0);
	ctx->PSSetShader(nullptr, nullptr, 0);

	/* Reset topology */
	ctx->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
}