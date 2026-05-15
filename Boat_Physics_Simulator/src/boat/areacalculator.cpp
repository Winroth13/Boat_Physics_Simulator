#include "boat/areacalculator.h"

#include "core/renderer/renderer.h"
#include "core/logger.h"

#include "graphics/camera.h"
#include "graphics/models/model.h"

constexpr UINT WIDTH = 256;
constexpr UINT HEIGHT = 256;

constexpr float WORLD_CAMERA_WIDTH = 6.0f;
constexpr float WORLD_CAMERA_HEIGHT = 6.0f;

AreaCalculator::AreaCalculator()
{
}

AreaCalculator::~AreaCalculator()
{
	if (mTextureRTV)
	{
		mTextureRTV->Release();
	}

	if (mTextureUAV)
	{
		mTextureUAV->Release();
	}

	if (mTextureSRV)
	{
		mTextureSRV->Release();
	}

	if (mTexture)
	{
		mTexture->Release();
	}

	if (mAreaBufferUAV)
	{
		mAreaBufferUAV->Release();
	}

	if (mAreaStagingBuffer)
	{
		mAreaStagingBuffer->Release();
	}

	if (mAreaBuffer)
	{
		mAreaBuffer->Release();
	}
}

bool AreaCalculator::Create()
{
	/* Configure Viewport */
	{
		mViewport.TopLeftX = 0;
		mViewport.TopLeftY = 0;
		mViewport.Width = WIDTH;
		mViewport.Height = HEIGHT;
		mViewport.MinDepth = 0;
		mViewport.MaxDepth = 1;
	}

	/* Texture */
	{
		D3D11_TEXTURE2D_DESC desc = {};
		desc.Width = WIDTH;
		desc.Height = HEIGHT;
		desc.MipLevels = 1;
		desc.ArraySize = 1;
		desc.Format = DXGI_FORMAT_R8_UNORM;
		desc.SampleDesc.Count = 1;
		desc.SampleDesc.Quality = 0;
		desc.Usage = D3D11_USAGE_DEFAULT;
		desc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_UNORDERED_ACCESS | D3D11_BIND_SHADER_RESOURCE;
		desc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
		desc.MiscFlags = 0;

		if (FAILED(Renderer::GetDevice()->CreateTexture2D(&desc, nullptr, &mTexture)))
		{
			Logger::Error("Failed to create area texture");
			return false;
		}
	}

	/* Create UAV */
	{
		D3D11_UNORDERED_ACCESS_VIEW_DESC desc = {};
		desc.Format = DXGI_FORMAT_R8_UNORM;
		desc.ViewDimension = D3D11_UAV_DIMENSION_TEXTURE2D;

		if (FAILED(Renderer::GetDevice()->CreateUnorderedAccessView(mTexture, &desc, &mTextureUAV)))
		{
			Logger::Error("Failed to create area uav");
			return false;
		}
	}

	/* Create SRV */
	{
		D3D11_SHADER_RESOURCE_VIEW_DESC desc = {};
		desc.Format = DXGI_FORMAT_R8_UNORM;
		desc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
		desc.Texture2D.MipLevels = 1;
		desc.Texture2D.MostDetailedMip = 0;

		if (FAILED(Renderer::GetDevice()->CreateShaderResourceView(mTexture, &desc, &mTextureSRV)))
		{
			Logger::Error("Failed to create area srv");
			return false;
		}
	}

	/* Create RTV */
	{
		D3D11_RENDER_TARGET_VIEW_DESC desc = {};
		desc.Format = DXGI_FORMAT_R8_UNORM;
		desc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
	
		if (FAILED(Renderer::GetDevice()->CreateRenderTargetView(mTexture, &desc, &mTextureRTV)))
		{
			Logger::Error("Failed to create area rtv");
			return false;
		}
    }

	mVertexShader = std::make_unique<VertexShader>("resources/VertexShader.cso");
	mPixelShader = std::make_unique<PixelShader>("resources/AreaPixelShader.cso");
	mComputeShader = std::make_unique<ComputeShader>("resources/AreaComputeShader.cso");

	/* Create Area Buffer */
	{
		D3D11_BUFFER_DESC desc = {};
		desc.ByteWidth = sizeof(float) * WIDTH * HEIGHT;
		desc.Usage = D3D11_USAGE_DEFAULT;
		desc.CPUAccessFlags = 0;
		desc.BindFlags = D3D11_BIND_UNORDERED_ACCESS;
		desc.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;
		desc.StructureByteStride = sizeof(float);

		if (FAILED(Renderer::GetDevice()->CreateBuffer(&desc, nullptr, &mAreaBuffer)))
		{
			Logger::Error("Failed to create structured buffer");
			return false;
		}

		D3D11_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
		uavDesc.Format = DXGI_FORMAT_UNKNOWN;
		uavDesc.ViewDimension = D3D11_UAV_DIMENSION_BUFFER;
		uavDesc.Buffer.FirstElement = 0;
		uavDesc.Buffer.NumElements = WIDTH * HEIGHT;

		if (FAILED(Renderer::GetDevice()->CreateUnorderedAccessView(mAreaBuffer, &uavDesc, &mAreaBufferUAV)))
		{
			Logger::Error("Failed to create area unordered acces view");
			return false;
		}
	}

	/* Create Area Staging Buffer */
	{
		D3D11_BUFFER_DESC desc = {};
		desc.ByteWidth = sizeof(float) * WIDTH * HEIGHT;
		desc.Usage = D3D11_USAGE_STAGING;
		desc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
		desc.BindFlags = 0;
		desc.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;
		desc.StructureByteStride = sizeof(float);

		if (FAILED(Renderer::GetDevice()->CreateBuffer(&desc, nullptr, &mAreaStagingBuffer)))
		{
			Logger::Error("Failed to create area staging buffer");
			return false;
		}
	}

	return true;
}

float AreaCalculator::CalculateArea(std::shared_ptr<Model> model, DirectX::XMMATRIX transform, DirectX::XMFLOAT3 angles)
{
	auto ctx = Renderer::GetContext();

	/* Render Mesh under Water */
	{
		Camera camera;
		camera.transform.SetAngles(angles);
		camera.SetOrthographicLens(WORLD_CAMERA_WIDTH, WORLD_CAMERA_HEIGHT, 0.1f, 10.0f);

		DirectX::XMFLOAT3 forwardDirf = camera.transform.GetForwardDir3f();
		DirectX::XMVECTOR lookDirV = DirectX::XMLoadFloat3(&forwardDirf);
		lookDirV = DirectX::XMVectorScale(lookDirV, -5);
		lookDirV = DirectX::XMVector3Transform(lookDirV, transform);
		camera.transform.SetPosition(lookDirV);

		camera.UpdateViewMatrix();

		auto& renderServer = Renderer::GetRenderServerStatic();

		float clearColor[4] = { 0.0, 0.0, 0.0, 1.0 };

		ctx->ClearRenderTargetView(mTextureRTV, clearColor);

		CameraData cameraData = {};
		cameraData.pos = camera.transform.GetPosition3f();
		cameraData.view = camera.GetView();
		cameraData.viewProj = camera.GetViewProj();
		renderServer.UpdatePerViewBuffer(&cameraData);

		renderServer.UpdatePerObject(transform);

		ctx->OMSetRenderTargets(1, &mTextureRTV, nullptr);
		ctx->RSSetViewports(1, &mViewport);

		/* Bind Shaders */
		{
			ctx->VSSetShader(mVertexShader->GetShader(), nullptr, 0);
			ctx->PSSetShader(mPixelShader->GetShader(), nullptr, 0);
		}

		/* Draw Mesh */
		{
			auto mesh = model->GetMesh(0);

			UINT stride = sizeof(Vertex);
			UINT offset = 0;

			ID3D11Buffer* vertexBuffer = mesh->GetVertexBuffer();
			ID3D11Buffer* indexBuffer = mesh->GetIndexBuffer();

			ctx->IASetVertexBuffers(0, 1, &vertexBuffer, &stride, &offset);
			ctx->IASetIndexBuffer(indexBuffer, DXGI_FORMAT_R32_UINT, 0);
			ctx->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

			ctx->DrawIndexed(static_cast<UINT>(mesh->GetNumIndicies()), 0, 0);
		}

		/* Cleanup */
		{
			ctx->VSSetShader(nullptr, 0, 0);
			ctx->PSSetShader(nullptr, 0, 0);
			ctx->OMSetRenderTargets(0, nullptr, nullptr);
		};
	}
	
	/* Compute area from texture */
	{
		ctx->CSSetShader(mComputeShader->GetShader(), nullptr, 0);
		ctx->CSSetShaderResources(0, 1, &mTextureSRV);
		ctx->CSSetUnorderedAccessViews(0, 1, &mAreaBufferUAV, nullptr);

		constexpr UINT groupsX = (WIDTH + 7) / 8;
		constexpr UINT groupsY = (HEIGHT + 7) / 8;
		ctx->Dispatch(groupsX, groupsY, 1);

		ctx->CSSetShader(nullptr, nullptr, 0);
	}

	ctx->CopyResource(mAreaStagingBuffer, mAreaBuffer);
	D3D11_MAPPED_SUBRESOURCE mappedResource;

	HRESULT hr = ctx->Map(mAreaStagingBuffer, 0, D3D11_MAP_READ, 0, &mappedResource);

    float sum = 0.0f;
	if (SUCCEEDED(hr))
	{
		float* dataPtr = reinterpret_cast<float*>(mappedResource.pData);
		for (int i = 0; i < WIDTH * HEIGHT; ++i)
			sum += dataPtr[i];
	}
	ctx->Unmap(mAreaStagingBuffer, 0);

	constexpr float texelArea = (WORLD_CAMERA_WIDTH / WIDTH) * (WORLD_CAMERA_HEIGHT / HEIGHT);
	float totalArea = sum * texelArea;

	//Logger::Info(std::to_string(totalArea));

	return totalArea;
}
