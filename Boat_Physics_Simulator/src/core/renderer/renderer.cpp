#include "core/renderer/renderer.h"
#include <iostream>
#include "core/logger.h"

#include <DirectXMath.h>
#include <unordered_map>

#include "graphics/textures/cubemaptexture.h"
#include "graphics/camera.h"
#include "math/mathfunctions.h"

constexpr unsigned int MAX_DIRECTIONAL_LIGHTS = 8;
constexpr unsigned int MAX_POINT_LIGHTS = 8;
constexpr unsigned int MAX_SPOT_LIGHTS = 8;

constexpr unsigned int MAX_MATERIALS = 64;

/* Constant Buffers */
constexpr UINT PER_FRAME = 0;
constexpr UINT PER_VIEW = 1;
constexpr UINT PER_OBJECT = 2;
constexpr UINT PER_MATERIAL = 3;
constexpr UINT MATERIAL_INDEX = 4;

/* Light Structured Buffers */
constexpr UINT DIRECTIONAL_LIGHT_SLOT = 0;
constexpr UINT POINT_LIGHT_SLOT = 1;
constexpr UINT SPOT_LIGHT_SLOT = 2;

/* Texture Slots */
constexpr UINT DIFFUSE_TEXTURE_SLOT = 3;
constexpr UINT CUBEMAP_TEXTURE_SLOT = 4;
constexpr UINT NORMALMAP_TEXTURE_SLOT = 5;
constexpr UINT DISPLACEMENT_TEXTURE_SLOT = 6;
constexpr UINT ALPHA_TEXTURE_SLOT = 7;

/* Shadow Maps */
constexpr UINT DIRECTIONAL_LIGHT_SHADOW_MAPS_SLOT = 8;
constexpr UINT POINT_LIGHT_SHADOW_MAPS_SLOT = 9;
constexpr UINT SPOT_LIGHT_SHADOW_MAPS_SLOT = 10;

/* Shadow Map Dimentions */
constexpr unsigned int DIRECTIONAL_SHADOW_MAP_DIMENTIONS = 4096;
constexpr unsigned int POINT_SHADOW_MAP_DIMENTIONS = 512;
constexpr unsigned int SPOT_SHADOW_MAP_DIMENTIONS = 512;

/* Materials */
constexpr UINT DEFERRED_MATERIALS_SLOT = 3;

/* GBuffers */
constexpr UINT GBUFFER_START_SLOT = 11;

/* Samplers */
constexpr UINT DEFAULT_SAMPLER_SLOT = 0;
constexpr UINT SHADOW_MAP_SAMPLER_SLOT = 1;

Renderer::Renderer() :
	mSwapChain(nullptr),
	mBackBufferRenderTargetView(nullptr),
	mWindow(nullptr),
	mClearColor({ 0,0,0,255 }),
	mPerFrameBuffer(nullptr),
	mPerObjectBuffer(nullptr),
	mPerViewBuffer(nullptr)
{
}

ID3D11Device* Renderer::sDevice = nullptr;
ID3D11DeviceContext* Renderer::mImmediateContext = nullptr;

Renderer::~Renderer()
{
}

bool Renderer::Create(DirectX::XMFLOAT4 clearColor, Window* window)
{
	mClearColor = clearColor;

	if (window == nullptr)
	{
		Logger::Error("Renderer was given an invalid window");
		return false;
	}

	mWindow = window;

	if (mRenderServer.Create(this) == false)
	{
		Logger::Error("Renderer failed to create RenderServer");
		return false;
	}

	/* Create Interface */
	{
		DXGI_SWAP_CHAIN_DESC swapChainDesc = {};

		swapChainDesc.BufferDesc.Width = window->Width();
		swapChainDesc.BufferDesc.Height = window->Height();
		swapChainDesc.BufferDesc.RefreshRate.Numerator = 0;
		swapChainDesc.BufferDesc.RefreshRate.Denominator = 1;
		swapChainDesc.BufferDesc.Format = DXGI_FORMAT::DXGI_FORMAT_R8G8B8A8_UNORM;
		swapChainDesc.BufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
		swapChainDesc.BufferDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;

		swapChainDesc.SampleDesc.Count = 1;
		swapChainDesc.SampleDesc.Quality = 0;

		swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT | DXGI_USAGE_UNORDERED_ACCESS;
		swapChainDesc.BufferCount = 2;
		swapChainDesc.OutputWindow = mWindow->GetWindowsWindow();
		swapChainDesc.Windowed = true;
		swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
		swapChainDesc.Flags = 0;

		UINT flags = 0;

#ifdef _DEBUG
		flags = D3D11_CREATE_DEVICE_DEBUG;
#endif

		D3D_FEATURE_LEVEL featurelevels[] = { D3D_FEATURE_LEVEL_11_0 };

		HRESULT hr = D3D11CreateDeviceAndSwapChain(
			nullptr,
			D3D_DRIVER_TYPE_HARDWARE,
			nullptr,
			flags,
			featurelevels,
			1,
			D3D11_SDK_VERSION,
			&swapChainDesc,
			&mSwapChain,
			&Renderer::sDevice,
			nullptr,
			&mImmediateContext
		);

		if (FAILED(hr))
		{
			Logger::Error("Failed to create device and swapchain");
			return false;
		}

		mImmediateContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	}

	/* Create Render Target View and Unordered Access View */
	{
		ID3D11Texture2D* backBuffer = nullptr;
		if (
			FAILED(mSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), reinterpret_cast<void**>(&backBuffer))))
		{
			Logger::Error("Failed to create render target");
			return false;
		}

		HRESULT hr = sDevice->CreateRenderTargetView(backBuffer, nullptr, &mBackBufferRenderTargetView);

		if (FAILED(hr))
		{
			Logger::Error("Failed to create render target view for the back buffer");
			return false;
		}

		D3D11_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
		uavDesc.ViewDimension = D3D11_UAV_DIMENSION_TEXTURE2DARRAY;
		uavDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		uavDesc.Texture2DArray.MipSlice = 0;
		uavDesc.Texture2DArray.ArraySize = 1;

		hr = sDevice->CreateUnorderedAccessView(backBuffer, &uavDesc, &mBackBufferUAV);
		backBuffer->Release();
		if (FAILED(hr))
		{
			Logger::Error("Failed to create unordered access view for the back buffer");
			return false;
		}
	}

	/* Create Default Sampler */
	{
		D3D11_SAMPLER_DESC samplerDesc = {};
		samplerDesc.Filter = D3D11_FILTER_ANISOTROPIC;
		samplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
		samplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
		samplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
		samplerDesc.MipLODBias = 0.0f;
		samplerDesc.MaxAnisotropy = D3D11_REQ_MAXANISOTROPY;
		samplerDesc.ComparisonFunc = D3D11_COMPARISON_ALWAYS;
		samplerDesc.MinLOD = 0;
		samplerDesc.MaxLOD = D3D11_FLOAT32_MAX;

		HRESULT hr = Renderer::GetDevice()->CreateSamplerState(&samplerDesc, &mDefaultSampler);

		if (FAILED(hr))
		{
			Logger::Error("Failed to create default sampler state");
			throw std::runtime_error("");
		}
	}

	/* Create Constant Buffers */
	{
		/* Per Frame */
		{
			PerFrameBuffer perFrameBuffer = {};
			D3D11_BUFFER_DESC perFrameBufferDesc = {};
			perFrameBufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
			perFrameBufferDesc.Usage = D3D11_USAGE_DEFAULT;
			perFrameBufferDesc.CPUAccessFlags = 0;
			perFrameBufferDesc.MiscFlags = 0;
			perFrameBufferDesc.ByteWidth = sizeof(perFrameBuffer);
			perFrameBufferDesc.StructureByteStride = 0;

			D3D11_SUBRESOURCE_DATA data = {};
			data.pSysMem = &perFrameBuffer;

			HRESULT hr = sDevice->CreateBuffer(&perFrameBufferDesc, &data, &mPerFrameBuffer);

			if (FAILED(hr))
			{
				Logger::Error("Failed to create per frame buffer");
				return false;
			}
		}

		/* Per Object */
		{
			PerObject perObjectBuffer = {};
			D3D11_BUFFER_DESC perObjectBufferDesc = {};
			perObjectBufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
			perObjectBufferDesc.Usage = D3D11_USAGE_DEFAULT;
			perObjectBufferDesc.CPUAccessFlags = 0;
			perObjectBufferDesc.MiscFlags = 0;
			perObjectBufferDesc.ByteWidth = sizeof(perObjectBuffer);
			perObjectBufferDesc.StructureByteStride = 0;

			D3D11_SUBRESOURCE_DATA data = {};
			data.pSysMem = &perObjectBuffer;

			HRESULT hr = sDevice->CreateBuffer(&perObjectBufferDesc, &data, &mPerObjectBuffer);

			if (FAILED(hr))
			{
				Logger::Error("Failed to create per object buffer");
				return false;
			}
		}

		/* Per View */
		{
			PerViewBuffer perViewBuffer = {};
			D3D11_BUFFER_DESC perViewBufferDesc = {};
			perViewBufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
			perViewBufferDesc.Usage = D3D11_USAGE_DEFAULT;
			perViewBufferDesc.CPUAccessFlags = 0;
			perViewBufferDesc.MiscFlags = 0;
			perViewBufferDesc.ByteWidth = sizeof(perViewBuffer);
			perViewBufferDesc.StructureByteStride = 0;

			D3D11_SUBRESOURCE_DATA data = {};
			data.pSysMem = &perViewBuffer;

			HRESULT hr = sDevice->CreateBuffer(&perViewBufferDesc, &data, &mPerViewBuffer);

			if (FAILED(hr))
			{
				Logger::Error("Failed to create per view buffer");
				return false;
			}
		}

		/* Material Index */
		{
			MaterialIndexBuffer materialIndexBuffer = {};
			D3D11_BUFFER_DESC materialIndexBufferDesc = {};
			materialIndexBufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
			materialIndexBufferDesc.Usage = D3D11_USAGE_DEFAULT;
			materialIndexBufferDesc.CPUAccessFlags = 0;
			materialIndexBufferDesc.MiscFlags = 0;
			materialIndexBufferDesc.ByteWidth = sizeof(materialIndexBuffer);
			materialIndexBufferDesc.StructureByteStride = 0;

			D3D11_SUBRESOURCE_DATA data = {};
			data.pSysMem = &materialIndexBuffer;

			HRESULT hr = sDevice->CreateBuffer(&materialIndexBufferDesc, &data, &mMaterialIndexBuffer);

			if (FAILED(hr))
			{
				Logger::Error("Failed to create material index buffer");
				return false;
			}
		}
	}

	/* Create Directional Light Buffers */
	{
		D3D11_BUFFER_DESC lightBufferDesc = {};
		lightBufferDesc.ByteWidth = sizeof(DirectionalLightData) * MAX_DIRECTIONAL_LIGHTS;
		lightBufferDesc.Usage = D3D11_USAGE_DYNAMIC;
		lightBufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
		lightBufferDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
		lightBufferDesc.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;
		lightBufferDesc.StructureByteStride = sizeof(DirectionalLightData);

		D3D11_SUBRESOURCE_DATA data;
		data.pSysMem = 0;
		data.SysMemPitch = 0;
		data.SysMemSlicePitch = 0;

		if (FAILED(sDevice->CreateBuffer(&lightBufferDesc, NULL, &mDirectionalLightsBuffer)))
		{
			Logger::Error("Failed to create directional lights buffer");
			return false;
		}

		D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
		srvDesc.Format = DXGI_FORMAT_UNKNOWN;
		srvDesc.ViewDimension = D3D11_SRV_DIMENSION_BUFFER;
		srvDesc.Buffer.FirstElement = 0;
		srvDesc.Buffer.NumElements = MAX_DIRECTIONAL_LIGHTS;

		if (FAILED(sDevice->CreateShaderResourceView(mDirectionalLightsBuffer, &srvDesc, &mDirectionalLightsSRV)))
		{
			Logger::Error("Failed to create directional lights shader resource view");
			return false;
		}

		mDirectionalLightsData.reserve(MAX_DIRECTIONAL_LIGHTS);
	}

	/* Create Point Light Buffers */
	{
		D3D11_BUFFER_DESC pointLightsBufferDesc = {};
		pointLightsBufferDesc.ByteWidth = sizeof(PointLightBuffer) * MAX_POINT_LIGHTS;
		pointLightsBufferDesc.Usage = D3D11_USAGE_DYNAMIC;
		pointLightsBufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
		pointLightsBufferDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
		pointLightsBufferDesc.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;
		pointLightsBufferDesc.StructureByteStride = sizeof(PointLightBuffer);

		D3D11_SUBRESOURCE_DATA data;
		data.pSysMem = 0;
		data.SysMemPitch = 0;
		data.SysMemSlicePitch = 0;

		if (FAILED(sDevice->CreateBuffer(&pointLightsBufferDesc, NULL, &mPointLightsBuffer)))
		{
			Logger::Error("Failed to create point lights buffer");
			return false;
		}

		D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
		srvDesc.Format = DXGI_FORMAT_UNKNOWN;
		srvDesc.ViewDimension = D3D11_SRV_DIMENSION_BUFFER;
		srvDesc.Buffer.FirstElement = 0;
		srvDesc.Buffer.NumElements = MAX_POINT_LIGHTS;

		if (FAILED(sDevice->CreateShaderResourceView(mPointLightsBuffer, &srvDesc, &mPointLightsSRV)))
		{
			Logger::Error("Failed to create point lights shader resource view");
			return false;
		}

		mPointLightsData.reserve(MAX_POINT_LIGHTS);
	}

	/* Create Spot Light Buffers */
	{
		D3D11_BUFFER_DESC lightBufferDesc = {};
		lightBufferDesc.ByteWidth = sizeof(SpotLightData) * MAX_SPOT_LIGHTS;
		lightBufferDesc.Usage = D3D11_USAGE_DYNAMIC;
		lightBufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
		lightBufferDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
		lightBufferDesc.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;
		lightBufferDesc.StructureByteStride = sizeof(SpotLightData);

		D3D11_SUBRESOURCE_DATA data;
		data.pSysMem = 0;
		data.SysMemPitch = 0;
		data.SysMemSlicePitch = 0;

		if (FAILED(sDevice->CreateBuffer(&lightBufferDesc, NULL, &mSpotLightsBuffer)))
		{
			Logger::Error("Failed to create spot lights buffer");
			return false;
		}

		D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
		srvDesc.Format = DXGI_FORMAT_UNKNOWN;
		srvDesc.ViewDimension = D3D11_SRV_DIMENSION_BUFFER;
		srvDesc.Buffer.FirstElement = 0;
		srvDesc.Buffer.NumElements = MAX_SPOT_LIGHTS;

		if (FAILED(sDevice->CreateShaderResourceView(mSpotLightsBuffer, &srvDesc, &mSpotLightsSRV)))
		{
			Logger::Error("Failed to create spot lights shader resource view");
			return false;
		}

		mSpotLightsData.reserve(MAX_SPOT_LIGHTS);
	}

	/* Create GBuffers */
	{
		if (!mGBuffers.Create(mWindow->Width(), mWindow->Height()))
		{
			Logger::Error("Failed to create gbuffers");
			return false;
		}
	}

	/* Create Materials Structured Buffer */
	{
		/* Per Material Buffer */
		{
			D3D11_BUFFER_DESC perMaterialBufferDesc = {};
			perMaterialBufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
			perMaterialBufferDesc.Usage = D3D11_USAGE_DEFAULT;
			perMaterialBufferDesc.CPUAccessFlags = 0;
			perMaterialBufferDesc.MiscFlags = 0;
			perMaterialBufferDesc.ByteWidth = sizeof(PerMaterial);
			perMaterialBufferDesc.StructureByteStride = 0;

			D3D11_SUBRESOURCE_DATA data = {};
			data.pSysMem = &perMaterialBufferDesc;

			HRESULT hr = sDevice->CreateBuffer(&perMaterialBufferDesc, &data, &mPerMaterialBuffer);

			if (FAILED(hr))
			{
				Logger::Error("Failed to create per material buffer");
				return false;
			}
		}

		/* Materials Structured Buffer */
		{
			D3D11_BUFFER_DESC perMaterialBufferDesc = {};
			perMaterialBufferDesc.ByteWidth = sizeof(PerMaterial) * MAX_MATERIALS;
			perMaterialBufferDesc.Usage = D3D11_USAGE_DYNAMIC;
			perMaterialBufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
			perMaterialBufferDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
			perMaterialBufferDesc.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;
			perMaterialBufferDesc.StructureByteStride = sizeof(PerMaterial);

			D3D11_SUBRESOURCE_DATA data;
			data.pSysMem = 0;
			data.SysMemPitch = 0;
			data.SysMemSlicePitch = 0;

			if (FAILED(sDevice->CreateBuffer(&perMaterialBufferDesc, NULL, &mMaterialsBuffer)))
			{
				Logger::Error("Failed to create materials structured buffer");
				return false;
			}

			D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
			srvDesc.Format = DXGI_FORMAT_UNKNOWN;
			srvDesc.ViewDimension = D3D11_SRV_DIMENSION_BUFFER;
			srvDesc.Buffer.FirstElement = 0;
			srvDesc.Buffer.NumElements = MAX_MATERIALS;

			if (FAILED(sDevice->CreateShaderResourceView(mMaterialsBuffer, &srvDesc, &mMaterialsSRV)))
			{
				Logger::Error("Failed to create materials shader resource view");
				return false;
			}
		}
	}

	/* Create Shadow Maps */
	{
		mShadowMapPixelShader = std::make_unique<PixelShader>("resources/ShadowMapPixelShader.cso");
		mShadowMapLinearPixelShader = std::make_unique<PixelShader>("resources/ShadowMapLinearPixelShader.cso");

		if (!mDirectionalLightsShadowMap.Create(
			MAX_DIRECTIONAL_LIGHTS,
			DIRECTIONAL_SHADOW_MAP_DIMENTIONS,
			DIRECTIONAL_SHADOW_MAP_DIMENTIONS
		))
		{
			Logger::Error("Failed to create directional lights shadow map");
			return false;
		}

		if (!mPointLightsShadowMap.Create(
			MAX_POINT_LIGHTS,
			POINT_SHADOW_MAP_DIMENTIONS,
			POINT_SHADOW_MAP_DIMENTIONS
		))
		{
			Logger::Error("Failed to create point lights shadow map");
			return false;
		}

		if (!mSpotLightsShadowMap.Create(
			MAX_SPOT_LIGHTS,
			SPOT_SHADOW_MAP_DIMENTIONS,
			SPOT_SHADOW_MAP_DIMENTIONS
		))
		{
			Logger::Error("Failed to create spot lights shadow map");
			return false;
		}

		/* Create Sampler */
		{
			D3D11_SAMPLER_DESC samplerDesc = {};
			samplerDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
			samplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_BORDER;
			samplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_BORDER;
			samplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_BORDER;
			samplerDesc.BorderColor[0] = 1.0f;
			samplerDesc.BorderColor[1] = 1.0f;
			samplerDesc.BorderColor[2] = 1.0f;
			samplerDesc.BorderColor[3] = 0.0f;
			samplerDesc.MipLODBias = 0.0f;
			samplerDesc.ComparisonFunc = D3D11_COMPARISON_ALWAYS;
			samplerDesc.MinLOD = 0;
			samplerDesc.MaxLOD = D3D11_FLOAT32_MAX;

			HRESULT hr = Renderer::GetDevice()->CreateSamplerState(&samplerDesc, &mShadowMapSampler);

			if (FAILED(hr))
			{
				Logger::Error("DirectX failed to create sampler state for shadow maps");
				return false;
			}
		}
	}

	/* Load Deferred Lighting Shader */
	{
		mLightingComputeShader = std::make_unique<ComputeShader>("resources/DeferredLightingComputeShader.cso");
	}

	/* Load Deferred Pixel Shader */
	{
		mDeferredPixelShader = std::make_unique<PixelShader>("resources/DeferredPixelShader.cso");
	}

	/* Load Tessellation Shaders */
	{
		mTessellationHullShader = std::make_unique<HullShader>("resources/TessellationHullShader.cso");
		mDisplacementDomainShader = std::make_unique<DomainShader>("resources/DisplacementDomainShader.cso");
	}

	if (!mAABBRenderer.Create())
	{
		Logger::Error("Failed to create AABB Renderer");
		return false;
	}

	if (!mLineRenderer.Create())
	{
		Logger::Error("Failed to create Line Renderer");
		return false;
	}

	if (!mParticleRenderer.Create())
	{
		Logger::Error("Failed to create Particle Renderer");
		return false;
	}

	if (!mSpriteRenderer.Create())
	{
		Logger::Error("Failed to create Sprite Renderer");
		return false;
	}

	mStaticGeometryTree.Create(20, 20, 20, 30, 5);

	/* Initialize Rasterizer Desc */
	mRasterizerDesc.CullMode = D3D11_CULL_BACK;
	mRasterizerDesc.FillMode = D3D11_FILL_SOLID;

	return true;
}

void Renderer::Shutdown()
{
	/* Release Light Buffers */
	{
		if (mDirectionalLightsBuffer != nullptr)
			mDirectionalLightsBuffer->Release();

		if (mDirectionalLightsSRV != nullptr)
			mDirectionalLightsSRV->Release();

		if (mPointLightsBuffer != nullptr)
			mPointLightsBuffer->Release();

		if (mPointLightsSRV != nullptr)
			mPointLightsSRV->Release();

		if (mSpotLightsBuffer != nullptr)
			mSpotLightsBuffer->Release();

		if (mSpotLightsSRV != nullptr)
			mSpotLightsSRV->Release();
	}

	if (mDefaultSampler != nullptr)
		mDefaultSampler->Release();

	if (mShadowMapSampler != nullptr)
		mShadowMapSampler->Release();

	if (mMaterialsBuffer != nullptr)
		mMaterialsBuffer->Release();

	if (mMaterialsSRV != nullptr)
		mMaterialsSRV->Release();

	if (mPerViewBuffer != nullptr)
		mPerViewBuffer->Release();

	if (mPerObjectBuffer != nullptr)
		mPerObjectBuffer->Release();

	if (mPerFrameBuffer != nullptr)
		mPerFrameBuffer->Release();

	if (mPerMaterialBuffer != nullptr)
		mPerMaterialBuffer->Release();

	if (mMaterialIndexBuffer != nullptr)
		mMaterialIndexBuffer->Release();

	if (mBackBufferRenderTargetView != nullptr)
		mBackBufferRenderTargetView->Release();

	if (mBackBufferUAV != nullptr)
		mBackBufferUAV->Release();

	if (mSwapChain != nullptr)
		mSwapChain->Release();

	if (mImmediateContext != nullptr)
		mImmediateContext->Release();

	if (sDevice != nullptr)
		sDevice->Release();
}

void Renderer::BeginRender()
{
	if ((mFlags & WIRE_FRAME) == WIRE_FRAME)
	{
		ID3D11RasterizerState* state;
		mRasterizerDesc.FillMode = D3D11_FILL_SOLID;
		mRasterizerDesc.CullMode = D3D11_CULL_BACK;

		sDevice->CreateRasterizerState(&mRasterizerDesc, &state);
		mImmediateContext->RSSetState(state);
		state->Release();
	}

	mFlags = mNewFlags;

	mTickCount = GetTickCount();

	BindPerFrameBuffer(ShaderType::VERTEX);

	BindPerViewBuffer(ShaderType::VERTEX);
	BindPerViewBuffer(ShaderType::PIXEL);
	BindPerViewBuffer(ShaderType::HULL);
	BindPerViewBuffer(ShaderType::DOMAIN_SHADER);
	BindPerViewBuffer(ShaderType::GEOMETRY);
	BindPerMaterialBuffer(ShaderType::PIXEL);

	mImmediateContext->PSSetSamplers(DEFAULT_SAMPLER_SLOT, 1, &mDefaultSampler);
	mImmediateContext->DSSetSamplers(DEFAULT_SAMPLER_SLOT, 1, &mDefaultSampler);
}

void Renderer::Render()
{
	RenderShadowMaps();
	RenderCubeMaps();

	if ((mNewFlags & WIRE_FRAME) == WIRE_FRAME)
	{
		ID3D11RasterizerState* state;
		mRasterizerDesc.FillMode = D3D11_FILL_WIREFRAME;
		mRasterizerDesc.CullMode = D3D11_CULL_BACK;

		sDevice->CreateRasterizerState(&mRasterizerDesc, &state);
		mImmediateContext->RSSetState(state);
		state->Release();
	}

	if ((mNewFlags & SHOW_BOUNDING_BOXES) == SHOW_BOUNDING_BOXES)
	{
		RenderBB();
	}

	RenderDeferred(mBackBufferUAV, mGBuffers, mFlags, mSceneCamera, mSceneFrustum);
}

void Renderer::BeginForward(ID3D11RenderTargetView* renderTarget, ID3D11DepthStencilView* depthStencilView)
{
	ID3D11RenderTargetView* rtv = renderTarget;
	if (renderTarget == nullptr)
	{
		rtv = mBackBufferRenderTargetView;
	}

	ID3D11DepthStencilView* dsv = depthStencilView;
	if (depthStencilView == nullptr)
	{
		dsv = mGBuffers.GetDSV();
	}

	mImmediateContext->OMSetRenderTargets(1, &rtv, dsv);
}

void Renderer::RenderForward()
{
	if (!((mFlags & RenderFlags::SHOW_GBUFFERS) == RenderFlags::SHOW_GBUFFERS))
	{
		if ((mFlags & RenderFlags::SHOW_ICONS) == RenderFlags::SHOW_ICONS)
		{
			mSpriteRenderer.Render(mImmediateContext, mRenderServer, mSpritesData);
		}

		mParticleRenderer.Render(mImmediateContext, mRenderServer, mParticleSystemsData);
		mAABBRenderer.Render(mImmediateContext, mAABBData);
		mLineRenderer.Render(mImmediateContext, mLineData);
	}
}

void Renderer::EndForward()
{
	ID3D11RenderTargetView* nullView = nullptr;
	mImmediateContext->OMSetRenderTargets(1, &nullView, mGBuffers.GetDSV());
}

void Renderer::PresentRender()
{
	ClearFrameData();
	mSwapChain->Present(0, 0);
}

void Renderer::UpdatePerFrameBuffer(
	const DirectX::XMFLOAT3 ambientColor,
	const uint32_t numDirectionalLights,
	const uint32_t numPointLights,
	const uint32_t numSpotLights,
	const uint32_t flags,
	const std::array<uint32_t, 2> screenDimensions
)
{
	PerFrameBuffer perFrameBuffer = {};
	perFrameBuffer.ambientColor = ambientColor;
	perFrameBuffer.numDirectionalLights = numDirectionalLights;
	perFrameBuffer.numPointLights = numPointLights;
	perFrameBuffer.numSpotLights = numSpotLights;
	perFrameBuffer.flags = flags;
	perFrameBuffer.tickCount = mTickCount;
	perFrameBuffer.screenDimensions = screenDimensions;
	perFrameBuffer.sceneCameraPos = mSceneCamera.pos;

	mImmediateContext->UpdateSubresource(mPerFrameBuffer, 0, NULL, &perFrameBuffer, 0, 0);
}

void Renderer::PushGeometryData(const GeometryData& geometryData, bool isStatic)
{
	if (isStatic)
	{
		mStaticGeometryData.push_back(geometryData);
	}
	else
	{
		mGeometryData.push_back(geometryData);
	}
}

void Renderer::PushMaterialData(const MaterialData& materialData, bool isStatic)
{
	if (isStatic)
	{
		mStaticMaterialData.push_back(materialData);
	}
	else
	{
		mMaterialData.push_back(materialData);
	}
}

void Renderer::PushDirectionalLightData(const DirectionalLightData& directionalLightData)
{
	if (mDirectionalLightsData.size() < MAX_DIRECTIONAL_LIGHTS)
	{
		mDirectionalLightsData.push_back(directionalLightData);
	}
	else
	{
		Logger::Warn("Directional lights cannot exceed " + std::to_string(MAX_DIRECTIONAL_LIGHTS));
	}
}

void Renderer::PushPointLightData(const PointLightData& pointLightData)
{
	if (mPointLightsData.size() < MAX_POINT_LIGHTS)
	{
		mPointLightsData.push_back(pointLightData);
	}
	else
	{
		Logger::Warn("Point lights cannot exceed " + std::to_string(MAX_POINT_LIGHTS));
	}
}

void Renderer::PushSpotLightData(const SpotLightData& spotLightData)
{
	if (mSpotLightsData.size() < MAX_SPOT_LIGHTS)
	{
		mSpotLightsData.push_back(spotLightData);
	}
	else
	{
		Logger::Warn("Spot lights cannot exceed " + std::to_string(MAX_SPOT_LIGHTS));
	}
}

void Renderer::PushCubemapData(const CubemapData& cubemapData)
{
	mCubemapsData.push_back(cubemapData);
}

void Renderer::PushAABBData(const AABBData& aabbData)
{
	mAABBData.push_back(aabbData);
}

void Renderer::PushLineData(const LineData& lineData)
{
	mLineData.push_back(lineData);
}

void Renderer::PushParticleSystemData(const ParticleSystemData& particleSystemData)
{
	mParticleSystemsData.push_back(particleSystemData);
}

void Renderer::PushSpriteData(const SpriteData& spriteData)
{
	mSpritesData.push_back(spriteData);
}

void Renderer::SetEnviromentData(const EnviromentData& enviromentData)
{
	mEnviromentData = enviromentData;
}

void Renderer::SetSceneCamera(const CameraData& cameraData)
{
	mSceneCamera = cameraData;
}

void Renderer::SetCullingFrustum(const FrustumData& frustum)
{
	mSceneFrustum = frustum;
}

void Renderer::BakeStaticGeometry()
{
	mStaticGeometryTree.Clear();

	for (size_t i = 0; i < mStaticGeometryData.size(); ++i)
	{
		auto& mesh = mStaticGeometryData[i].mesh;
		auto& transform = mStaticGeometryData[i].transform;

		AABB localBounds = mesh->GetBounds();
		AABB bounds = localBounds.Transform(transform);

		mStaticGeometryTree.AddElement(i, bounds.ToBoundingBox());
	}
}

void Renderer::RenderShadowMaps()
{
	/* Directional Lights */
	{
		/* Set Shadow Map Viewport for Directional Lights */
		mImmediateContext->RSSetViewports(1, &mDirectionalLightsShadowMap.GetViewport());

		for (size_t i = 0; i < mDirectionalLightsData.size(); ++i)
		{
			ID3D11DepthStencilView* dsv = mDirectionalLightsShadowMap.GetDSV(i);
			mImmediateContext->ClearDepthStencilView(dsv, D3D11_CLEAR_DEPTH, 1, 0);
			mImmediateContext->OMSetRenderTargets(0, nullptr, dsv);

			DirectionalLightData& dirLight = mDirectionalLightsData[i];

			Camera camera;
			camera.SetOrthographicLens(100, 100, 0.1f, 200.0f); // TODO: experimentation got us these values, idk!
			DirectX::XMFLOAT3 direction = dirLight.direction;
			camera.transform.SetAngles(DirectionToAngles(direction));

			direction.x *= -50;
			direction.y *= -50;
			direction.z *= -50;
			camera.transform.SetPosition(direction);

			// Center on the scene camera
			// Truncating is to remove the shimmering effect due to moving the shadow-texture all the time
			camera.transform.MoveX((float)(int)mSceneCamera.pos.x);
			camera.transform.MoveY((float)(int)mSceneCamera.pos.y);
			camera.transform.MoveZ((float)(int)mSceneCamera.pos.z);

			camera.UpdateViewMatrix();

			dirLight.viewProj = camera.GetViewProj();

			UpdatePerViewBuffer(
				{
					dirLight.viewProj,
					camera.GetView(),
					camera.transform.GetPosition3f()
				}
			);

			/* Draw static geometry to depth stencil */
			for (size_t i = 0; i < mStaticGeometryData.size(); ++i)
			{
				auto& geometryData = mStaticGeometryData[i];
				auto& materialData = mStaticMaterialData[i];

				if (materialData.material->HasAlpha())
				{
					BindPixelShader(mShadowMapPixelShader);
				}
				else
				{
					UnbindPixelShader();
				}

				RenderShadowMeshAndMaterial(geometryData, materialData);
			}

			/* Draw dynamic data to depth stencil */
			for (size_t i = 0; i < mGeometryData.size(); ++i)
			{
				auto& geometryData = mGeometryData[i];
				auto& materialData = mMaterialData[i];

				if (materialData.material->HasAlpha())
				{
					BindPixelShader(mShadowMapPixelShader);
				}
				else
				{
					UnbindPixelShader();
				}

				RenderShadowMeshAndMaterial(geometryData, materialData);
			}
		}
	}

	/* Point Lights */
	{
		BindPixelShader(mShadowMapLinearPixelShader);

		constexpr DirectX::XMFLOAT3 CAMERA_CUBEMAP_ANGLES[] =
		{
			{0, DirectX::XM_PI / 2, 0},					// Pos X
			{0, -DirectX::XM_PI / 2, 0},				// Neg X
			{-DirectX::XM_PI / 2, 0, 0},				// Pos Y
			{DirectX::XM_PI / 2, 0, 0},					// Neg Y
			{0, 0, 0},									// Pos Z
			{0, DirectX::XM_PI, 0},						// Neg Z
		};

		mImmediateContext->RSSetViewports(1, &mPointLightsShadowMap.GetViewport());
		ID3D11DepthStencilView* dsv = mPointLightsShadowMap.GetDSV();

		Camera camera;
		camera.SetPerspectiveLens(DirectX::XM_PI / 2, 1, 0.1f, 100.0f);

		for (size_t i = 0; i < mPointLightsData.size(); ++i)
		{
			auto& pointLightData = mPointLightsData[i];

			/* Only draw shadows if told so! */
			if (!pointLightData.updateShadows)
			{
				continue;
			}

			/* Move camera to center of cubemap */
			camera.transform.SetPosition(pointLightData.position);

			for (size_t face = 0; face < 6; ++face)
			{
				ID3D11RenderTargetView* rtv = mPointLightsShadowMap.GetRTV(i, face);

				constexpr float DEPTH_CLEAR_COLOR[4] = { 1, 1, 1, 1 };

				mImmediateContext->ClearRenderTargetView(rtv, DEPTH_CLEAR_COLOR);
				mImmediateContext->ClearDepthStencilView(dsv, D3D11_CLEAR_DEPTH, 1, 0);
				mImmediateContext->OMSetRenderTargets(1, &rtv, dsv);

				/* Orient camera to face */
				camera.transform.SetAngles(CAMERA_CUBEMAP_ANGLES[face]);
				camera.UpdateViewMatrix();

				CameraData cameraData = {};
				cameraData.pos = camera.transform.GetPosition3f();
				cameraData.view = camera.GetView();
				cameraData.viewProj = camera.GetViewProj();

				UpdatePerViewBuffer(cameraData);

				DirectX::BoundingFrustum frustum = camera.GetBoundingFrustum();

				/* Draw static geometry to depth stencil */
				for (auto& i : mStaticGeometryTree.GetVisibleElements(frustum))
				{
					GeometryData& geometryData = mStaticGeometryData[i];
					if (IsGeometryVisible(geometryData, frustum))
					{
						MaterialData& materialData = mStaticMaterialData[i];
						RenderShadowMeshAndMaterial(geometryData, materialData);
					}
				}

				/* Draw dynamic data to depth stencil */
				for (size_t i = 0; i < mGeometryData.size(); ++i)
				{
					GeometryData& geometryData = mGeometryData[i];
					if (IsGeometryVisible(geometryData, frustum))
					{
						MaterialData& materialData = mMaterialData[i];
						RenderShadowMeshAndMaterial(geometryData, materialData);
					}
				}
			}
		}
	}

	/* Spot Lights */
	{
		mImmediateContext->RSSetViewports(1, &mSpotLightsShadowMap.GetViewport());

		for (size_t i = 0; i < mSpotLightsData.size(); ++i)
		{
			SpotLightData& spotLight = mSpotLightsData[i];
			ID3D11DepthStencilView* dsv = mSpotLightsShadowMap.GetDSV(i);
			mImmediateContext->ClearDepthStencilView(dsv, D3D11_CLEAR_DEPTH, 1, 0);
			mImmediateContext->OMSetRenderTargets(0, nullptr, dsv);

			float aspectRatio = mSpotLightsShadowMap.GetWidth() / (float)mSpotLightsShadowMap.GetHeight();

			/* Update View Buffer */
			Camera camera;
			camera.SetPerspectiveLens(spotLight.angle * 2, aspectRatio, 0.1f, 100.0f);
			Transform transform;
			transform.SetAngles(DirectionToAngles(spotLight.direction));
			transform.SetPosition(spotLight.position);
			camera.transform = transform;
			camera.UpdateViewMatrix();

			spotLight.viewProj = camera.GetViewProj();

			UpdatePerViewBuffer(
				{
					spotLight.viewProj,
					camera.GetView(),
					camera.transform.GetPosition3f()
				}
			);

			DirectX::BoundingFrustum frustum = camera.GetBoundingFrustum();

			/* Draw static geometry to depth stencil */
			for (auto& i : mStaticGeometryTree.GetVisibleElements(frustum))
			{
				GeometryData& geometryData = mStaticGeometryData[i];
				if (IsGeometryVisible(geometryData, frustum))
				{
					MaterialData& materialData = mStaticMaterialData[i];

					if (materialData.material->HasAlpha())
					{
						BindPixelShader(mShadowMapPixelShader);
					}
					else
					{
						UnbindPixelShader();
					}

					RenderShadowMeshAndMaterial(geometryData, materialData);
				}
			}

			/* Draw dynamic data to depth stencil */
			for (size_t i = 0; i < mGeometryData.size(); ++i)
			{
				GeometryData& geometryData = mGeometryData[i];
				if (IsGeometryVisible(geometryData, frustum))
				{
					MaterialData& materialData = mMaterialData[i];

					if (materialData.material->HasAlpha())
					{
						BindPixelShader(mShadowMapPixelShader);
					}
					else
					{
						UnbindPixelShader();
					}

					RenderShadowMeshAndMaterial(geometryData, materialData);
				}
			}
		}
	}

	UnbindPixelShader();
}

void Renderer::RenderCubeMaps()
{
	BindPerViewBuffer(ShaderType::VERTEX);
	BindPerViewBuffer(ShaderType::PIXEL);

	constexpr DirectX::XMFLOAT3 CAMERA_CUBEMAP_ANGLES[] =
	{
		{0, DirectX::XM_PI / 2, 0},					// Pos X
		{0, -DirectX::XM_PI / 2, 0},				// Neg X
		{-DirectX::XM_PI / 2, 0, 0},				// Pos Y
		{DirectX::XM_PI / 2, 0, 0},					// Neg Y
		{0, 0, 0},									// Pos Z
		{0, DirectX::XM_PI, 0},						// Neg Z
	};

	Camera camera;

	/* Render to all cubemaps */
	for (size_t i = 0; i < mCubemapsData.size(); ++i)
	{
		auto& cubemapData = mCubemapsData[i];
		camera.SetPerspectiveLens(DirectX::XM_PI / 2, 1, 0.1f, 100.0f);

		/* Render to each side */
		for (size_t j = 0; j < 6; ++j)
		{
			uint16_t cubemapFlags = mFlags;
			cubemapFlags = (cubemapFlags &= ~WIRE_FRAME);
			cubemapFlags = (cubemapFlags &= ~SHOW_GBUFFERS);

			Transform transform;
			transform.SetPosition(cubemapData.position);
			transform.SetAngles(CAMERA_CUBEMAP_ANGLES[j]);

			camera.transform = transform;
			camera.UpdateViewMatrix();

			CameraData cameraData = {};
			cameraData.pos = camera.transform.GetPosition3f();
			cameraData.viewProj = camera.GetViewProj();
			cameraData.view = camera.GetView();

			FrustumData frustumData = {};
			frustumData.frustum = camera.GetBoundingFrustum();

			RenderDeferred(
				cubemapData.cubemapTexture->GetUAV(j),
				cubemapData.cubemapTexture->GetGBuffers(),
				cubemapFlags,
				cameraData,
				frustumData
			);

			BeginForward(
				cubemapData.cubemapTexture->GetRTV(j),
				cubemapData.cubemapTexture->GetGBuffers().GetDSV()
			);
			mParticleRenderer.Render(mImmediateContext, mRenderServer, mParticleSystemsData);
			EndForward();
		}
	}
}

void Renderer::RenderDeferred(ID3D11UnorderedAccessView* backBuffer, GBuffers& buffers, uint16_t flags, CameraData camera, FrustumData frustum)
{
	mImmediateContext->ClearUnorderedAccessViewFloat(
		backBuffer,
		&mClearColor.x
	);

	mImmediateContext->ClearRenderTargetView(
		buffers.GetRTV(GBufferType::COLOR),
		&mClearColor.x
	);

	DirectX::XMFLOAT4 noObject = { 0.0f, 0.0f, 0.0f, 0.0f };
	mImmediateContext->ClearRenderTargetView(
		buffers.GetRTV(GBufferType::POSITION),
		&noObject.x
	);

	mImmediateContext->ClearDepthStencilView(buffers.GetDSV(), D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1, 0);
	mImmediateContext->RSSetViewports(1, &buffers.GetViewport());

	/* Setup */
	{
		BindGBuffers(buffers);

		UpdatePerFrameBuffer(
			mEnviromentData.ambientColor,
			(uint32_t)mDirectionalLightsData.size(),
			(uint32_t)mPointLightsData.size(),
			(uint32_t)mSpotLightsData.size(),
			flags,
			{
				(uint32_t)mWindow->Width(),
				(uint32_t)mWindow->Height()
			}
		);

		BindPerFrameBuffer(ShaderType::COMPUTE);
		BindPerViewBuffer(ShaderType::COMPUTE);

		BindDirectionalLights(ShaderType::COMPUTE);
		BindPointLights(ShaderType::COMPUTE);
		BindSpotLights(ShaderType::COMPUTE);

		BindPixelShader(mDeferredPixelShader);
	}

	std::vector<PerMaterial> materials;

	/* Draw */
	{
		UpdatePerViewBuffer(camera);

		materials.reserve(MAX_MATERIALS);
		std::unordered_map<std::shared_ptr<Material>, uint32_t> materialsMap;

		/* Draw each mesh and material pair */

		/* Static entities */
		for (auto& staticIndicies : mStaticGeometryTree.GetVisibleElements(frustum.frustum))
		{
			auto& geometryData = mStaticGeometryData[staticIndicies];
			auto& materialData = mStaticMaterialData[staticIndicies];

			if (IsGeometryVisible(geometryData, frustum.frustum))
			{
				RenderDeferredMeshAndMaterial(geometryData, materialData, materialsMap, materials);
			}
		}

		/* Dynamic entities */
		for (size_t i = 0; i < mGeometryData.size(); ++i)
		{
			auto& geometryData = mGeometryData[i];
			auto& materialData = mMaterialData[i];

			if (IsGeometryVisible(geometryData, frustum.frustum))
			{
				RenderDeferredMeshAndMaterial(geometryData, materialData, materialsMap, materials);
			}
		}

		/* Fill materials buffer */
		{
			D3D11_MAPPED_SUBRESOURCE mappedResource;
			HRESULT result = mImmediateContext->Map(mMaterialsBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource);
			if (FAILED(result))
			{
				Logger::Error("Failed to map materials buffer");
				throw std::runtime_error("");
			}

			size_t numBytes = materials.size() * sizeof(PerMaterial);
			memcpy_s(mappedResource.pData, numBytes, materials.data(), numBytes);
			mImmediateContext->Unmap(mMaterialsBuffer, 0);
		}
	}

	/* End */
	{
		UnbindGBuffers();

		/* Compute Deferred Lighting */
		{
			mImmediateContext->CSSetShader(mLightingComputeShader->GetShader(), nullptr, 0);

			/* Bind GBuffer Shader Views */
			mImmediateContext->CSSetShaderResources(
				GBUFFER_START_SLOT,
				static_cast<UINT>(GBufferType::MAX),
				buffers.GetSRVs().data()
			);

			/* Bind Render Target */
			mImmediateContext->CSSetUnorderedAccessViews(0, 1, &backBuffer, nullptr);

			/* Bind Materials Shader Views */
			mImmediateContext->CSSetShaderResources(DEFERRED_MATERIALS_SLOT, 1, &mMaterialsSRV);

			/* Bind Shadow Sampler */
			mImmediateContext->CSSetSamplers(SHADOW_MAP_SAMPLER_SLOT, 1, &mShadowMapSampler);

			/* Bind Directional Light Shadow Maps */
			size_t nDirLights = mDirectionalLightsData.size();
			if (nDirLights > 0)
			{
				ID3D11ShaderResourceView* dirLightsSRV = mDirectionalLightsShadowMap.GetSRV(nDirLights);
				mImmediateContext->CSSetShaderResources(DIRECTIONAL_LIGHT_SHADOW_MAPS_SLOT, 1, &dirLightsSRV);
			}

			/* Bind Point Light Shadow Maps */
			size_t nPointLights = mPointLightsData.size();
			if (nPointLights > 0)
			{
				ID3D11ShaderResourceView* pointLightsSRV = mPointLightsShadowMap.GetSRV(nPointLights);
				mImmediateContext->CSSetShaderResources(POINT_LIGHT_SHADOW_MAPS_SLOT, 1, &pointLightsSRV);
			}

			/* Bind Spot Light Shadow Maps */
			size_t nSpotlights = mSpotLightsData.size();
			if (nSpotlights > 0)
			{
				ID3D11ShaderResourceView* spotLightsSRV = mSpotLightsShadowMap.GetSRV(nSpotlights);
				mImmediateContext->CSSetShaderResources(SPOT_LIGHT_SHADOW_MAPS_SLOT, 1, &spotLightsSRV);
			}

			/* Dispatch Compute Shader */
			UINT threadGroupCountX = static_cast<UINT>(buffers.GetWidth()) / 8;
			UINT threadGroupCountY = static_cast<UINT>(buffers.GetHeight()) / 8;
			mImmediateContext->Dispatch(threadGroupCountX, threadGroupCountY, 1);

			/* Unbind Light Shadow Maps */
			ID3D11ShaderResourceView* views = { nullptr };
			mImmediateContext->CSSetShaderResources(SPOT_LIGHT_SHADOW_MAPS_SLOT, 1, &views);
			mImmediateContext->CSSetShaderResources(POINT_LIGHT_SHADOW_MAPS_SLOT, 1, &views);
			mImmediateContext->CSSetShaderResources(DIRECTIONAL_LIGHT_SHADOW_MAPS_SLOT, 1, &views);

			/* Unbind GBuffer Shader Views */
			ID3D11ShaderResourceView* nullShaderViews[static_cast<UINT>(GBufferType::MAX)] = { nullptr };

			mImmediateContext->CSSetShaderResources(
				GBUFFER_START_SLOT,
				static_cast<UINT>(GBufferType::MAX),
				nullShaderViews
			);

			/* Unbind Render Target */
			ID3D11UnorderedAccessView* nullUAView{};
			mImmediateContext->CSSetUnorderedAccessViews(0, 1, &nullUAView, nullptr);

			/* Unbind Materials Shader Views */
			mImmediateContext->CSSetShaderResources(DEFERRED_MATERIALS_SLOT, 1, nullShaderViews);
		}
	}
}

void Renderer::RenderBB()
{
	/* Draw Bounding Boxes */
	constexpr DirectX::XMFLOAT3 RENDERED_MESH_BOUNDING_BOX_COLOR = { 0.0f, 1.0f, 0.0f };
	constexpr DirectX::XMFLOAT3 HIDDEN_MESH_BOUNDING_BOX_COLOR = { 1.0f, 0.0f, 0.0f };
	constexpr DirectX::XMFLOAT3 FRUSTUM_BOUNDING_BOX_COLOR = { 0.0f, 0.0f, 1.0f };

	/* Draw Geometry Bounding Boxes	*/
	for (size_t i = 0; i < mGeometryData.size() + mStaticGeometryData.size(); ++i)
	{
		std::shared_ptr<Mesh> mesh;
		DirectX::XMMATRIX transform;

		if (i < mGeometryData.size())
		{
			mesh = mGeometryData[i].mesh;
			transform = mGeometryData[i].transform;
		}
		else
		{
			mesh = mStaticGeometryData[i - mGeometryData.size()].mesh;
			transform = mStaticGeometryData[i - mGeometryData.size()].transform;
		}

		AABB localBounds = mesh->GetBounds();
		AABB bounds = localBounds.Transform(transform);

		DirectX::BoundingBox boundingBox = bounds.ToBoundingBox();
		DirectX::BoundingFrustum frustum = mSceneFrustum.frustum;

		DirectX::XMFLOAT3 color = RENDERED_MESH_BOUNDING_BOX_COLOR;
		if (!frustum.Intersects(boundingBox))
		{
			color = HIDDEN_MESH_BOUNDING_BOX_COLOR;
		}

		mRenderServer.PushAABB(bounds, color);
	}

	/* Draw Camera Frustum */
	DirectX::XMFLOAT3 corners[8];
	DirectX::BoundingFrustum visibleFrustrum = mSceneFrustum.frustum;
	visibleFrustrum.Far = visibleFrustrum.Far / 2; // This is for some reason the threshold for the far plane to be visible.
	visibleFrustrum.GetCorners(corners);

	// Near plane
	mRenderServer.PushLine(corners[0], corners[1], FRUSTUM_BOUNDING_BOX_COLOR);
	mRenderServer.PushLine(corners[1], corners[2], FRUSTUM_BOUNDING_BOX_COLOR);
	mRenderServer.PushLine(corners[2], corners[3], FRUSTUM_BOUNDING_BOX_COLOR);
	mRenderServer.PushLine(corners[3], corners[0], FRUSTUM_BOUNDING_BOX_COLOR);

	// Far plane
	mRenderServer.PushLine(corners[4], corners[5], FRUSTUM_BOUNDING_BOX_COLOR);
	mRenderServer.PushLine(corners[5], corners[6], FRUSTUM_BOUNDING_BOX_COLOR);
	mRenderServer.PushLine(corners[6], corners[7], FRUSTUM_BOUNDING_BOX_COLOR);
	mRenderServer.PushLine(corners[7], corners[4], FRUSTUM_BOUNDING_BOX_COLOR);

	// Between planes
	mRenderServer.PushLine(corners[0], corners[4], FRUSTUM_BOUNDING_BOX_COLOR);
	mRenderServer.PushLine(corners[1], corners[5], FRUSTUM_BOUNDING_BOX_COLOR);
	mRenderServer.PushLine(corners[2], corners[6], FRUSTUM_BOUNDING_BOX_COLOR);
	mRenderServer.PushLine(corners[3], corners[7], FRUSTUM_BOUNDING_BOX_COLOR);

	constexpr DirectX::XMFLOAT3 treeBoundsColors[6] =
	{
		{1, 0, 0},
		{0, 1, 0},
		{0, 0, 1},
		{1, 1, 0},
		{1, 0, 1},
		{0, 0, 0}
	};

	/* Draw Quad Tree */
	for (auto& result : mStaticGeometryTree.GetTreeBounds())
	{
		AABB aabb = AABB(result.boundingBox);
		mRenderServer.PushAABB(aabb, treeBoundsColors[result.depth % 6]);
	}
}

bool Renderer::IsGeometryVisible(GeometryData& geometryData, const DirectX::BoundingFrustum& frustum)
{
	DirectX::XMMATRIX& transform = geometryData.transform;

	AABB aabb = geometryData.mesh->GetBounds();
	aabb = aabb.Transform(transform);

	DirectX::BoundingBox boundingBox = aabb.ToBoundingBox();

	return frustum.Intersects(boundingBox);
}

void Renderer::RenderShadowMeshAndMaterial(GeometryData& geometryData, MaterialData& materialData)
{
	auto& mat = materialData.material;
	BindMaterialSRV(mat, 0);

	BindMesh(geometryData.mesh);
	UpdatePerObjectBuffer(geometryData.transform);
	UpdatePerMaterialBuffer(mat);
	mImmediateContext->DrawIndexed((UINT)geometryData.mesh->GetNumIndicies(), 0, 0);

	/* Disable Tessellation */
	mImmediateContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	UnbindHullShader();
	UnbindDomainShader();
}

void Renderer::RenderDeferredMeshAndMaterial(
	GeometryData& geometryData,
	MaterialData& materialData,
	std::unordered_map<std::shared_ptr<Material>, uint32_t>& materialsMap,
	std::vector<PerMaterial>& perMaterials)
{
	auto& mesh = geometryData.mesh;
	auto& mat = materialData.material;
	DirectX::XMMATRIX& transform = geometryData.transform;

	if (materialsMap.find(mat) == materialsMap.end())
	{
		materialsMap[mat] = (uint32_t)perMaterials.size();

		PerMaterial data = {};
		data.ambientCoefficient = mat->GetAmbientCoefficient3f();
		data.diffuseCoefficient = mat->GetDiffuseCoefficient3f();
		data.specularCoefficient = mat->GetSpecularCoefficient3f();
		data.phongExponent = mat->GetPhongExponent();
		data.reflectiveness = mat->GetReflectiveness();

		if (perMaterials.size() < MAX_MATERIALS)
			perMaterials.emplace_back(data);
		else
			Logger::Warn("Materials cannot exceed " + MAX_MATERIALS);
	}

	uint32_t matIndex = materialsMap[mat];

	BindMesh(mesh);
	BindMaterialSRV(mat, matIndex);

	UpdatePerObjectBuffer(geometryData.transform);
	UpdatePerMaterialBuffer(mat);

	mImmediateContext->DrawIndexed((UINT)mesh->GetNumIndicies(), 0, 0);

	ID3D11ShaderResourceView* views[] = { nullptr };
	mImmediateContext->PSSetShaderResources(CUBEMAP_TEXTURE_SLOT, 1, views);

	/* Disable Tessellation */
	mImmediateContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	UnbindHullShader();
	UnbindDomainShader();
}

void Renderer::UpdatePerViewBuffer(const CameraData& cameraData)
{
	PerViewBuffer perViewBuffer = {};
	perViewBuffer.cameraPos = cameraData.pos;
	perViewBuffer.viewProj = cameraData.viewProj;
	perViewBuffer.view = cameraData.view;
	mImmediateContext->UpdateSubresource(mPerViewBuffer, 0, NULL, &perViewBuffer, 0, 0);
}

void Renderer::UpdatePerObjectBuffer(const DirectX::XMMATRIX world)
{
	PerObject perObjectBuffer = {};
	perObjectBuffer.world = DirectX::XMMatrixTranspose(world);
	perObjectBuffer.worldInverseTranspose = DirectX::XMMatrixTranspose(DirectX::XMMatrixInverse(nullptr, world));
	mImmediateContext->UpdateSubresource(mPerObjectBuffer, 0, NULL, &perObjectBuffer, 0, 0);

	mImmediateContext->VSSetConstantBuffers(BUFFER_PER_OBJECT, 1, &mPerObjectBuffer);
	mImmediateContext->PSSetConstantBuffers(BUFFER_PER_OBJECT, 1, &mPerObjectBuffer);
	mImmediateContext->GSSetConstantBuffers(BUFFER_PER_OBJECT, 1, &mPerObjectBuffer);
}

void Renderer::UpdatePerMaterialBuffer(std::shared_ptr<Material> material)
{
	PerMaterial perMatBuffer = {};
	perMatBuffer.ambientCoefficient = material->GetAmbientCoefficient3f();
	perMatBuffer.diffuseCoefficient = material->GetDiffuseCoefficient3f();
	perMatBuffer.specularCoefficient = material->GetSpecularCoefficient3f();
	perMatBuffer.phongExponent = material->GetPhongExponent();
	perMatBuffer.reflectiveness = material->GetReflectiveness();
	perMatBuffer.materialFlags = material->GetFlags();
	perMatBuffer.maxTessFactor = material->GetMaxTessFactor();
	perMatBuffer.maxTessDistance = material->GetMaxTessDistance();
	perMatBuffer.minTessDistance = material->GetMinTessDistance();
	perMatBuffer.dispStrength = material->SetDispStrength();

	mImmediateContext->UpdateSubresource(mPerMaterialBuffer, 0, NULL, &perMatBuffer, 0, 0);
}

void Renderer::BindGBuffers(GBuffers& buffers)
{
	mImmediateContext->OMSetRenderTargets(
		static_cast<UINT>(GBufferType::MAX),
		buffers.GetRTVs().data(),
		buffers.GetDSV()
	);
}

void Renderer::UnbindGBuffers()
{
	mImmediateContext->OMSetRenderTargets(0, nullptr, nullptr);
}

void Renderer::BindPerFrameBuffer(ShaderType shaderType)
{
	switch (shaderType)
	{
	case ShaderType::VERTEX:
		mImmediateContext->VSSetConstantBuffers(PER_FRAME, 1, &mPerFrameBuffer);
		break;
	case ShaderType::PIXEL:
		mImmediateContext->PSSetConstantBuffers(PER_FRAME, 1, &mPerFrameBuffer);
		break;
	case ShaderType::HULL:
		mImmediateContext->HSSetConstantBuffers(PER_FRAME, 1, &mPerFrameBuffer);
		break;
	case ShaderType::COMPUTE:
		mImmediateContext->CSSetConstantBuffers(PER_FRAME, 1, &mPerFrameBuffer);
		break;
	default:
		Logger::Warn("Trying to bind per frame buffer to an invalid shader type");
		break;
	}
}

void Renderer::BindPerViewBuffer(ShaderType shaderType)
{
	switch (shaderType)
	{
	case ShaderType::VERTEX:
		mImmediateContext->VSSetConstantBuffers(PER_VIEW, 1, &mPerViewBuffer);
		break;
	case ShaderType::PIXEL:
		mImmediateContext->PSSetConstantBuffers(PER_VIEW, 1, &mPerViewBuffer);
		break;
	case ShaderType::COMPUTE:
		mImmediateContext->CSSetConstantBuffers(PER_VIEW, 1, &mPerViewBuffer);
		break;
	case ShaderType::HULL:
		mImmediateContext->HSSetConstantBuffers(PER_VIEW, 1, &mPerViewBuffer);
		break;
	case ShaderType::DOMAIN_SHADER:
		mImmediateContext->DSSetConstantBuffers(PER_VIEW, 1, &mPerViewBuffer);
		break;
	case ShaderType::GEOMETRY:
		mImmediateContext->GSSetConstantBuffers(PER_VIEW, 1, &mPerViewBuffer);
		break;
	default:
		Logger::Warn("Trying to bind per frame buffer to an invalid shader type");
		break;
	}
}

void Renderer::BindPerMaterialBuffer(ShaderType shaderType)
{
	switch (shaderType)
	{
	case ShaderType::VERTEX:
		mImmediateContext->VSSetConstantBuffers(PER_MATERIAL, 1, &mPerMaterialBuffer);
		break;
	case ShaderType::HULL:
		mImmediateContext->HSSetConstantBuffers(PER_MATERIAL, 1, &mPerMaterialBuffer);
		break;
	case ShaderType::DOMAIN_SHADER:
		mImmediateContext->DSSetConstantBuffers(PER_MATERIAL, 1, &mPerMaterialBuffer);
		break;
	case ShaderType::PIXEL:
		mImmediateContext->PSSetConstantBuffers(PER_MATERIAL, 1, &mPerMaterialBuffer);
		break;
	case ShaderType::COMPUTE:
		mImmediateContext->CSSetConstantBuffers(PER_MATERIAL, 1, &mPerMaterialBuffer);
		break;
	default:
		Logger::Warn("Trying to bind per material buffer to an invalid shader type");
		break;
	}
}

void Renderer::BindMaterialSRV(std::shared_ptr<Material> material, uint32_t index)
{
	BindVertexShader(material->GetVertexShader());
	BindTexture2D(material->GetTexture(), DIFFUSE_TEXTURE_SLOT);
	BindTexture2D(material->GetCubemapTexture(), CUBEMAP_TEXTURE_SLOT);
	BindTexture2D(material->GetNormalMap(), NORMALMAP_TEXTURE_SLOT);
	BindTexture2D(material->GetDisplacementMap(), DISPLACEMENT_TEXTURE_SLOT);
	BindTexture2D(material->GetAlphaMap(), ALPHA_TEXTURE_SLOT);

	D3D11_CULL_MODE wishCullMode = material->HasAlpha() ? D3D11_CULL_NONE : D3D11_CULL_BACK;
	if (mRasterizerDesc.CullMode != wishCullMode)
	{
		ID3D11RasterizerState* state;
		mRasterizerDesc.CullMode = wishCullMode;

		sDevice->CreateRasterizerState(&mRasterizerDesc, &state);
		mImmediateContext->RSSetState(state);
		state->Release();
	}

	if (material->HasDisplacement())
	{
		mImmediateContext->HSSetConstantBuffers(BUFFER_PER_OBJECT, 1, &mPerObjectBuffer);
		mImmediateContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_3_CONTROL_POINT_PATCHLIST);
		BindHullShader(mTessellationHullShader);
		BindPerFrameBuffer(ShaderType::HULL);
		BindPerMaterialBuffer(ShaderType::HULL);
		BindPerMaterialBuffer(ShaderType::DOMAIN_SHADER);
		BindDomainShader(mDisplacementDomainShader);
		BindTexture2D(material->GetDisplacementMap(), DISPLACEMENT_TEXTURE_SLOT, ShaderType::DOMAIN_SHADER);
	}

	mImmediateContext->IASetInputLayout(material->GetInputLayout());

	UpdateMaterialIndexBuffer(index);
	mImmediateContext->PSSetConstantBuffers(MATERIAL_INDEX, 1, &mMaterialIndexBuffer);
}

void Renderer::BindMesh(std::shared_ptr<Mesh> mesh)
{
	UINT stride = sizeof(Vertex);
	UINT offset = 0;

	ID3D11Buffer* vertexBuffer = mesh->GetVertexBuffer();
	ID3D11Buffer* indexBuffer = mesh->GetIndexBuffer();

	mImmediateContext->IASetVertexBuffers(0, 1, &vertexBuffer, &stride, &offset);
	mImmediateContext->IASetIndexBuffer(indexBuffer, DXGI_FORMAT_R32_UINT, 0);
}

void Renderer::BindVertexShader(std::shared_ptr<VertexShader> vertexShader)
{
	mImmediateContext->VSSetShader(
		vertexShader->GetShader(),
		nullptr,
		0
	);
}

void Renderer::BindVertexShader(const std::unique_ptr<VertexShader>& vertexShader)
{
	mImmediateContext->VSSetShader(
		vertexShader->GetShader(),
		nullptr,
		0
	);
}

void Renderer::BindPixelShader(std::shared_ptr<PixelShader> pixelShader)
{
	mImmediateContext->PSSetShader(
		pixelShader->GetShader(),
		nullptr,
		0
	);
}

void Renderer::BindPixelShader(const std::unique_ptr<PixelShader>& pixelShader)
{
	mImmediateContext->PSSetShader(
		pixelShader->GetShader(),
		nullptr,
		0
	);
}

void Renderer::BindDomainShader(std::shared_ptr<DomainShader> domainShader)
{
	mImmediateContext->DSSetShader(domainShader->GetShader(), nullptr, 0);
}

void Renderer::BindDomainShader(const std::unique_ptr<DomainShader>& domainShader)
{
	mImmediateContext->DSSetShader(domainShader->GetShader(), nullptr, 0);
}

void Renderer::BindHullShader(std::shared_ptr<HullShader> hullShader)
{
	mImmediateContext->HSSetShader(hullShader->GetShader(), nullptr, 0);
}

void Renderer::BindHullShader(const std::unique_ptr<HullShader>& hullShader)
{
	mImmediateContext->HSSetShader(hullShader->GetShader(), nullptr, 0);
}

void Renderer::BindTexture2D(std::shared_ptr<Texture2D> texture2d, UINT slot, ShaderType type)
{
	if (texture2d)
	{
		ID3D11ShaderResourceView* srv = texture2d->GetSRV();
		if (type == ShaderType::PIXEL)
		{
			mImmediateContext->PSSetShaderResources(slot, 1, &srv);
		}
		else if (type == ShaderType::DOMAIN_SHADER)
		{
			mImmediateContext->DSSetShaderResources(slot, 1, &srv);
		}
		else
		{
			Logger::Warn(
				"Trying to bind texture to an unsupported shader type: "
				+ std::to_string((int)type)
			);
		}
	}
	else
	{
		ID3D11ShaderResourceView* views[] = { nullptr };
		mImmediateContext->PSSetShaderResources(slot, 1, views);
	}
}

void Renderer::BindDirectionalLights(ShaderType shaderType)
{
	D3D11_MAPPED_SUBRESOURCE mappedResource;
	HRESULT result = mImmediateContext->Map(mDirectionalLightsBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource);
	if (FAILED(result))
	{
		Logger::Error("Failed to map directional lights buffer");
		throw std::runtime_error("");
	}

	size_t numBytes = mDirectionalLightsData.size() * sizeof(DirectionalLightData);
	memcpy_s(mappedResource.pData, numBytes, mDirectionalLightsData.data(), numBytes);
	mImmediateContext->Unmap(mDirectionalLightsBuffer, 0);

	switch (shaderType)
	{
	case ShaderType::PIXEL:
		mImmediateContext->PSSetShaderResources(DIRECTIONAL_LIGHT_SLOT, 1, &mDirectionalLightsSRV);
		break;
	case ShaderType::COMPUTE:
		mImmediateContext->CSSetShaderResources(DIRECTIONAL_LIGHT_SLOT, 1, &mDirectionalLightsSRV);
		break;
	default:
		Logger::Warn("Trying to bind directional lights to an invalid shader type");
		break;
	}
}

void Renderer::BindPointLights(ShaderType shaderType)
{
	D3D11_MAPPED_SUBRESOURCE mappedResource;
	HRESULT result = mImmediateContext->Map(mPointLightsBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource);
	if (FAILED(result))
	{
		Logger::Error("Failed to map point lights buffer");
		throw std::runtime_error("");
	}

	size_t numBytes = mPointLightsData.size() * sizeof(PointLightBuffer);

	std::vector<PointLightBuffer> pointLightBuffers;
	pointLightBuffers.reserve(mPointLightsData.size());
	for (auto& data : mPointLightsData)
	{
		PointLightBuffer buffer = {};
		buffer.attenuation = data.attenuation;
		buffer.color = data.color;
		buffer.intensity = data.intensity;
		buffer.position = data.position;
		pointLightBuffers.emplace_back(buffer);
	}

	memcpy_s(mappedResource.pData, numBytes, pointLightBuffers.data(), numBytes);
	mImmediateContext->Unmap(mPointLightsBuffer, 0);

	switch (shaderType)
	{
	case ShaderType::PIXEL:
		mImmediateContext->PSSetShaderResources(POINT_LIGHT_SLOT, 1, &mPointLightsSRV);
		break;
	case ShaderType::COMPUTE:
		mImmediateContext->CSSetShaderResources(POINT_LIGHT_SLOT, 1, &mPointLightsSRV);
		break;
	default:
		Logger::Warn("Trying to bind point lights to an invalid shader type");
		break;
	}
}

void Renderer::BindSpotLights(ShaderType shaderType)
{
	D3D11_MAPPED_SUBRESOURCE mappedResource;
	HRESULT result = mImmediateContext->Map(mSpotLightsBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource);
	if (FAILED(result))
	{
		Logger::Error("Failed to map spot lights buffer");
		throw std::runtime_error("");
	}

	size_t numBytes = mSpotLightsData.size() * sizeof(SpotLightData);
	memcpy_s(mappedResource.pData, numBytes, mSpotLightsData.data(), numBytes);
	mImmediateContext->Unmap(mSpotLightsBuffer, 0);

	switch (shaderType)
	{
	case ShaderType::PIXEL:
		mImmediateContext->PSSetShaderResources(SPOT_LIGHT_SLOT, 1, &mSpotLightsSRV);
		break;
	case ShaderType::COMPUTE:
		mImmediateContext->CSSetShaderResources(SPOT_LIGHT_SLOT, 1, &mSpotLightsSRV);
		break;
	default:
		Logger::Warn("Trying to bind spot lights to an invalid shader type");
		break;
	}
}

void Renderer::UnbindMaterial()
{
	UnbindVertexShader();
	UnbindPixelShader();
}

void Renderer::UnbindMesh()
{
	mImmediateContext->IASetVertexBuffers(0, 1, nullptr, 0, nullptr);
	mImmediateContext->IASetIndexBuffer(nullptr, DXGI_FORMAT_R32_UINT, 0);
}

void Renderer::UnbindVertexShader()
{
	mImmediateContext->VSSetShader(nullptr, nullptr, 0);
}

void Renderer::UnbindPixelShader()
{
	mImmediateContext->PSSetShader(nullptr, nullptr, 0);
}

void Renderer::UnbindDomainShader()
{
	mImmediateContext->DSSetShader(nullptr, nullptr, 0);
}

void Renderer::UnbindHullShader()
{
	mImmediateContext->HSSetShader(nullptr, nullptr, 0);
}

void Renderer::UnbindTexture2D(UINT slot)
{
	mImmediateContext->PSSetSamplers(slot, 1, nullptr);
	mImmediateContext->PSSetShaderResources(slot, 1, nullptr);
}

void Renderer::UpdateMaterialIndexBuffer(uint32_t index)
{
	MaterialIndexBuffer data = {};
	data.materialIndex = index;
	mImmediateContext->UpdateSubresource(mMaterialIndexBuffer, 0, NULL, &data, 0, 0);
}

void Renderer::ClearFrameData()
{
	/* Lights */
	{
		mDirectionalLightsData.clear();
		mSpotLightsData.clear();
		mPointLightsData.clear();
	}

	/* Particle Systems */
	{
		mParticleSystemsData.clear();
	}

	/* Sprites */
	{
		mSpritesData.clear();
	}

	/* Cubemaps */
	{
		mCubemapsData.clear();
	}

	/* Debug Drawing */
	{
		mAABBData.clear();
		mLineData.clear();
	}

	mGeometryData.clear();
	mMaterialData.clear();
}