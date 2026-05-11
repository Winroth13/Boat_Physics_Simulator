#include "graphics/particlesystem.h"
#include "core/logger.h"
#include "core/renderer/renderer.h"
#include "graphics/shaders/computeshader.h"
#include "core/engine.h"

#include <iostream>
#include <d3d11.h>

ParticleSystem::ParticleSystem(uint32_t count)
{
	mParticleCount = count;
	mComputeShader = std::make_unique<ComputeShader>("resources/ParticleComputeShader.cso");

	CreateParticleBuffer(count);

	/* Create Particle System Buffer */
	{
		D3D11_BUFFER_DESC desc = {};
		desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
		desc.Usage = D3D11_USAGE_DEFAULT;
		desc.CPUAccessFlags = 0;
		desc.MiscFlags = 0;
		desc.ByteWidth = sizeof(ParticleSystemBuffer);
		desc.StructureByteStride = 0;

		ParticleSystemBuffer particleSystemBuffer = {};

		D3D11_SUBRESOURCE_DATA data = {};
		data.pSysMem = &particleSystemBuffer;

		HRESULT hr = Renderer::GetDevice()->CreateBuffer(&desc, &data, &mParticleSystemBuffer);

		if (FAILED(hr))
		{
			Logger::Error("Failed to create particle system constant buffer");
			throw std::runtime_error("");
		}
	}
}

ParticleSystem::~ParticleSystem()
{
	if (mUAV)
	{
		mUAV->Release();
	}

	if (mSRV)
	{
		mSRV->Release();
	}

	if (mParticleBuffer)
	{
		mParticleBuffer->Release();
	}

	if (mParticleSystemBuffer)
	{
		mParticleSystemBuffer->Release();
	}
}

void ParticleSystem::Update(double deltaTime)
{
	constexpr UINT THREAD_GROUPS_COUNT = 32;

	ID3D11DeviceContext* ctx = Renderer::GetContext();

	/* Bind Compute Shader */
	ctx->CSSetShader(mComputeShader->GetShader(), nullptr, 0);

	/* Bind Particle UAV */
	ctx->CSSetUnorderedAccessViews(0, 1, &mUAV, nullptr);

	/* Bind Particle System Buffer */
	ctx->CSSetConstantBuffers(0, 1, &mParticleSystemBuffer);

	mParticleSystemConstants.ticks = Engine::GetTicks();
	mParticleSystemConstants.deltaTime = static_cast<float>(deltaTime);
	ctx->UpdateSubresource(mParticleSystemBuffer, 0, NULL, &mParticleSystemConstants, 0, 0);

	UINT threads = static_cast<UINT>(std::ceil(mParticleCount / (float)THREAD_GROUPS_COUNT) * THREAD_GROUPS_COUNT);
	ctx->Dispatch(threads, 1, 1);

	/* Unbind Particle System Buffer */
	ID3D11Buffer* nullBuffer = nullptr;
	ctx->CSSetConstantBuffers(0, 1, &nullBuffer);

	/* Unbind Particle UAV */
	ID3D11UnorderedAccessView* nullUAV = nullptr;
	ctx->CSSetUnorderedAccessViews(0, 1, &nullUAV, nullptr);

	/* Unbind Compute Shader */
	ctx->CSSetShader(nullptr, nullptr, 0);

	/* Uncheck Reset */
	mParticleSystemConstants.flags &= ~static_cast<uint32_t>(ParticleSystemFlags::RESET);
}

bool ParticleSystem::IsAnimated()
{
	return (mParticleSystemConstants.flags & static_cast<uint32_t>(ParticleSystemFlags::ANIMATED)) == static_cast<uint32_t>(ParticleSystemFlags::ANIMATED);
}

bool ParticleSystem::IsDesaturate()
{
	return (mParticleSystemConstants.flags & static_cast<uint32_t>(ParticleSystemFlags::DESATURATE)) == static_cast<uint32_t>(ParticleSystemFlags::DESATURATE);
}

bool ParticleSystem::IsAdditive()
{
	return (mParticleSystemConstants.flags & static_cast<uint32_t>(ParticleSystemFlags::ADDITIVE)) == static_cast<uint32_t>(ParticleSystemFlags::ADDITIVE);
}

void ParticleSystem::SetCount(uint32_t count)
{
	mParticleCount = count;
	CreateParticleBuffer(count);
}

void ParticleSystem::SetDesaturate(bool enabled)
{
	if (enabled)
	{
		mParticleSystemConstants.flags |= static_cast<uint32_t>(ParticleSystemFlags::DESATURATE);
	}
	else
	{
		mParticleSystemConstants.flags &= ~static_cast<uint32_t>(ParticleSystemFlags::DESATURATE);
	}
}

void ParticleSystem::SetAnimated(bool enabled)
{
	if (enabled)
	{
		mParticleSystemConstants.flags |= static_cast<uint32_t>(ParticleSystemFlags::ANIMATED);
	}
	else
	{
		mParticleSystemConstants.flags &= ~static_cast<uint32_t>(ParticleSystemFlags::ANIMATED);
	}
}

void ParticleSystem::SetAdditive(bool enabled)
{
	if (enabled)
	{
		mParticleSystemConstants.flags |= static_cast<uint32_t>(ParticleSystemFlags::ADDITIVE);
	}
	else
	{
		mParticleSystemConstants.flags &= ~static_cast<uint32_t>(ParticleSystemFlags::ADDITIVE);
	}
}

void ParticleSystem::Reset()
{
	mParticleSystemConstants.flags |= static_cast<uint32_t>(ParticleSystemFlags::RESET);
}

void ParticleSystem::CreateParticleBuffer(uint32_t count)
{
	if (mParticleBuffer)
	{
		mUAV->Release();
		mSRV->Release();
		mParticleBuffer->Release();
	}

	/* Create Particle Buffer */
	{
		D3D11_BUFFER_DESC desc = {};
		desc.ByteWidth = sizeof(ParticleBuffer) * static_cast<UINT>(count);
		desc.Usage = D3D11_USAGE_DEFAULT;
		desc.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_UNORDERED_ACCESS;
		desc.CPUAccessFlags = 0;
		desc.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;
		desc.StructureByteStride = sizeof(ParticleBuffer);

		ParticleBuffer* particleBufferData = new ParticleBuffer[count];
		D3D11_SUBRESOURCE_DATA data = {};
		data.pSysMem = particleBufferData;
		data.SysMemPitch = data.SysMemSlicePitch = 0;

		HRESULT hr = Renderer::GetDevice()->CreateBuffer(&desc, &data, &mParticleBuffer);

		if (FAILED(hr))
		{
			Logger::Error("Failed to create particle buffer");
			throw std::runtime_error("");
		}

		delete[] particleBufferData;
	}

	/* Create SRV */
	{
		D3D11_SHADER_RESOURCE_VIEW_DESC desc = {};
		desc.Format = DXGI_FORMAT_UNKNOWN;
		desc.ViewDimension = D3D11_SRV_DIMENSION_BUFFER;
		desc.Buffer.FirstElement = 0;
		desc.Buffer.NumElements = static_cast<UINT>(count);

		HRESULT hr = Renderer::GetDevice()->CreateShaderResourceView(mParticleBuffer, &desc, &mSRV);

		if (FAILED(hr))
		{
			Logger::Error("Failed to create particle shader resource view");
			throw std::runtime_error("");
		}
	}

	/* Create UAV */
	{
		D3D11_UNORDERED_ACCESS_VIEW_DESC desc = {};
		desc.Format = DXGI_FORMAT_UNKNOWN;
		desc.ViewDimension = D3D11_UAV_DIMENSION_BUFFER;
		desc.Buffer.FirstElement = 0;
		desc.Buffer.NumElements = static_cast<UINT>(count);
		desc.Buffer.Flags = 0;

		HRESULT hr = Renderer::GetDevice()->CreateUnorderedAccessView(mParticleBuffer, &desc, &mUAV);

		if (FAILED(hr))
		{
			Logger::Error("Failed to create particle unordered access view");
			throw std::runtime_error("");
		}
	}
}
