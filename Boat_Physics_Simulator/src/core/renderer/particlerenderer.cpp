#include "core/renderer/particlerenderer.h"

#include "graphics/shaders/computeshader.h"
#include "graphics/shaders/geometryshader.h"
#include "graphics/shaders/vertexshader.h"
#include "graphics/shaders/pixelshader.h"
#include "graphics/textures/texture2d.h"
#include "core/renderer/renderserver.h"

#include "core/renderer/renderer.h"

#include "math/transform.h"

#include <DirectXMath.h>

ParticleRenderer::ParticleRenderer()
{
}

ParticleRenderer::~ParticleRenderer()
{
}

bool ParticleRenderer::Create()
{
	mParticleVertexShader = std::make_unique<VertexShader>("resources/ParticleVertexShader.cso");
	mParticlePixelShader = std::make_unique<PixelShader>("resources/ParticlePixelShader.cso");
	mBillboardGeometryShader = std::make_unique<GeometryShader>("resources/BillboardGeometryShader.cso");

	return true;
}

void ParticleRenderer::Render(
	ID3D11DeviceContext* ctx,
	RenderServer& renderServer,
	std::vector<ParticleSystemData>& data
)
{
	constexpr UINT PARTICLE_COLOR_TEXTURE_SLOT = 0;
	constexpr UINT PARTICLE_ALPHA_TEXTURE_SLOT = 1;

	if (data.size() < 1)
	{
		return;
	}

	/* Create Additive Blend State and Depth Stencil State */
	ID3D11BlendState* blendState = nullptr;
	ID3D11DepthStencilState* depthStencilState = nullptr;
	UINT sampleMask = 0xffffffff;
	float blendFactor[4] = { 0.8f, 0.8f, 0.8f, 0.0f };
	
	/* Blend State */
	D3D11_BLEND_DESC desc = {};
	desc.RenderTarget[0].BlendEnable = true;
	desc.RenderTarget[0].SrcBlend = D3D11_BLEND_ONE;
	desc.RenderTarget[0].DestBlend = D3D11_BLEND_ONE;
	desc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
	desc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
	desc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ONE;
	desc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
	desc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
	Renderer::GetDevice()->CreateBlendState(&desc, &blendState);

	/* Depth Stencil State */
	D3D11_DEPTH_STENCIL_DESC dsDesc = {};
	dsDesc.DepthEnable = true;
	dsDesc.DepthFunc = D3D11_COMPARISON_LESS;
	dsDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;
	dsDesc.StencilEnable = false;
	Renderer::GetDevice()->CreateDepthStencilState(&dsDesc, &depthStencilState);
	

	/* Configure Input Assembler */
	ctx->IASetInputLayout(nullptr);
	ctx->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_POINTLIST);

	/* Bind Shaders */
	ctx->VSSetShader(mParticleVertexShader->GetShader(), nullptr, 0);
	ctx->PSSetShader(mParticlePixelShader->GetShader(), nullptr, 0);
	ctx->GSSetShader(mBillboardGeometryShader->GetShader(), nullptr, 0);

	for (ParticleSystemData& p : data)
	{
		/* Modify Output Merger when Additive */
		if (p.system->IsAdditive())
		{
			ctx->OMSetDepthStencilState(depthStencilState, 0);
			ctx->OMSetBlendState(blendState, blendFactor, sampleMask);
		}
		else
		{
			ctx->OMSetBlendState(nullptr, blendFactor, sampleMask);
			ctx->OMSetDepthStencilState(nullptr, 0);
		}

		renderServer.UpdatePerObject(p.transform);

		/* Bind Particle System Buffer */
		ID3D11Buffer* systemBuffer = p.system->GetParticleSystemBuffer();
		ctx->GSSetConstantBuffers(0, 1, &systemBuffer);
		ctx->PSSetConstantBuffers(0, 1, &systemBuffer);

		/* Bind Textures */
		ID3D11ShaderResourceView* colorTexView = p.system->GetColorTexture()->GetSRV();
		ID3D11ShaderResourceView* alphaTexView = p.system->GetAlphaTexture()->GetSRV();
		ctx->PSSetShaderResources(PARTICLE_COLOR_TEXTURE_SLOT, 1, &colorTexView);
		ctx->PSSetShaderResources(PARTICLE_ALPHA_TEXTURE_SLOT, 1, &alphaTexView);

		ID3D11ShaderResourceView* srv = p.system->GetSRV();
		ctx->VSSetShaderResources(0, 1, &srv);

		ctx->Draw(static_cast<UINT>(p.system->GetCount()), 0);
	}

	/* Unbind Particle System Buffer */
	ID3D11Buffer* nullBuffer = nullptr;
	ctx->GSSetConstantBuffers(0, 1, &nullBuffer);
	ctx->PSSetConstantBuffers(0, 1, &nullBuffer);

	/* Unbind Textures */
	ID3D11ShaderResourceView* nullSrv = nullptr;
	ctx->PSSetShaderResources(PARTICLE_COLOR_TEXTURE_SLOT, 1, &nullSrv);
	ctx->PSSetShaderResources(PARTICLE_ALPHA_TEXTURE_SLOT, 1, &nullSrv);

	/* Unbind SRV */
	ID3D11ShaderResourceView* srvNull = nullptr;
	ctx->VSSetShaderResources(0, 1, &srvNull);

	/* Unbind Shaders */
	ctx->VSSetShader(nullptr, nullptr, 0);
	ctx->PSSetShader(nullptr, nullptr, 0);
	ctx->GSSetShader(nullptr, nullptr, 0);

	blendState->Release();
	depthStencilState->Release();

	/* Reset Output Merger */
	ctx->OMSetBlendState(nullptr, blendFactor, sampleMask);
	ctx->OMSetDepthStencilState(nullptr, 0);

	ctx->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
}