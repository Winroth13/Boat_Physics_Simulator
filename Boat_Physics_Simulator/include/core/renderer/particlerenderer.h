#pragma once
#include "math/transform.h"
#include "graphics/particlesystem.h"

#include <memory>
#include <vector>

class PixelShader;
class VertexShader;
class ComputeShader;
class GeometryShader;
class RenderServer;

struct ID3D11DeviceContext;

struct ParticleSystemData
{
	std::shared_ptr<ParticleSystem> system;
	DirectX::XMMATRIX transform;
};

class ParticleRenderer
{
public:
	ParticleRenderer();
	~ParticleRenderer();

	bool Create();
	void Render(
		ID3D11DeviceContext* ctx,
		RenderServer& renderServer,
		std::vector<ParticleSystemData>& data
	);

private:
	std::unique_ptr<VertexShader> mParticleVertexShader;
	std::unique_ptr<PixelShader> mParticlePixelShader;
	std::unique_ptr<GeometryShader> mBillboardGeometryShader;
};