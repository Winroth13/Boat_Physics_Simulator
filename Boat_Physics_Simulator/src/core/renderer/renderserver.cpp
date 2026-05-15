#include "core/renderer/renderserver.h"
#include "core/renderer/renderer.h"
#include "core/logger.h"

#include "graphics/models/model.h"
#include "graphics/enviroment.h"

#include "math/transform.h"
#include "graphics/camera.h"

#include <iostream>
#include <d3d11.h>

bool RenderServer::Create(Renderer* renderer)
{
	mRenderer = renderer;
	if (renderer == nullptr)
	{
		Logger::Error("Failed to create RenderServer because renderer was invalid");
		return false;
	}
	return true;
}

void RenderServer::PushMesh(std::shared_ptr<Mesh> mesh, DirectX::XMMATRIX transform, bool isStatic)
{
	GeometryData data = {};
	data.mesh = mesh;
	data.transform = transform;
	mRenderer->PushGeometryData(data, isStatic);
}

void RenderServer::PushMaterial(std::shared_ptr<Material> material, bool isStatic)
{
	MaterialData data = {};
	data.material = material;
	mRenderer->PushMaterialData(data, isStatic);
}

void RenderServer::PushDirectionalLight(DirectX::XMFLOAT3 direction, DirectX::XMFLOAT3 color, float intensity)
{
	DirectionalLightData data = {};
	data.direction = direction;
	data.color = color;
	data.intensity = intensity;
	mRenderer->PushDirectionalLightData(data);
}

void RenderServer::PushPointLight(
	DirectX::XMFLOAT3 position,
	DirectX::XMFLOAT3 color,
	float attenuation,
	float intensity,
	bool updateShadows
)
{
	PointLightData data = {};
	data.position = position;
	data.color = color;
	data.attenuation = attenuation;
	data.intensity = intensity;
	data.updateShadows = updateShadows;
	mRenderer->PushPointLightData(data);
}

void RenderServer::PushSpotLight(DirectX::XMFLOAT3 direction, DirectX::XMFLOAT3 position, DirectX::XMFLOAT3 color, float angle, float intensity, float attenuation)
{
	SpotLightData data = {};
	data.direction = direction;
	data.position = position;
	data.color = color;
	data.angle = angle;
	data.intensity = intensity;
	data.attenuation = attenuation;
	mRenderer->PushSpotLightData(data);
}

void RenderServer::PushCubemap(DirectX::XMFLOAT3 position, std::shared_ptr<CubemapTexture> cubemapTexture)
{
	CubemapData data = {};
	data.position = position;
	data.cubemapTexture = cubemapTexture;
	mRenderer->PushCubemapData(data);
}

void RenderServer::PushAABB(AABB aabb, DirectX::XMFLOAT3 color)
{
	DirectX::BoundingBox bounds = aabb.ToBoundingBox();
	AABBData data = {};
	data.origin = bounds.Center;
	data.size = bounds.Extents;
	data.color = color;
	mRenderer->PushAABBData(data);
}

void RenderServer::PushLine(DirectX::XMFLOAT3 start, DirectX::XMFLOAT3 end, DirectX::XMFLOAT3 color)
{
	LineData data = {};
	data.start = start;
	data.end = end;
	data.color = color;
	mRenderer->PushLineData(data);
}

void RenderServer::PushParticleSystem(std::shared_ptr<ParticleSystem> particleSystem, const DirectX::XMMATRIX transform)
{
	ParticleSystemData data = {};
	data.system = particleSystem;
	data.transform = transform;
	mRenderer->PushParticleSystemData(data);
}

void RenderServer::PushSprite(
	std::shared_ptr<Texture2D> texture, 
	const DirectX::XMMATRIX transform,
	const float scale,
	const DirectX::XMFLOAT3 tint
)
{
	SpriteData data = {};
	data.texture = texture;
	data.transform = transform;
	data.scale = scale;
	data.tint = tint;
	mRenderer->PushSpriteData(data);
}

void RenderServer::UpdateCamera(
	const DirectX::XMMATRIX viewProj,
	const DirectX::XMMATRIX view,
	const DirectX::XMFLOAT3 cameraPos
)
{
	CameraData data = {};
	data.viewProj = viewProj;
	data.pos = cameraPos;
	data.view = view;
	mRenderer->SetSceneCamera(data);
}

void RenderServer::UpdateFrustum(DirectX::BoundingFrustum frustum)
{
	FrustumData data = {};
	data.frustum = frustum;
	mRenderer->SetCullingFrustum(data);
}

void RenderServer::UpdateEnviroment(Enviroment& enviroment)
{
	EnviromentData data = {};
	data.ambientColor = enviroment.GetAmbientColor();
	data.useBlinnPhong = enviroment.IsUsingBlinnPhong();
	mRenderer->SetEnviromentData(data);
}

void RenderServer::UpdatePerObject(DirectX::XMMATRIX world)
{
	mRenderer->UpdatePerObjectBuffer(world);
}

void RenderServer::UpdatePerViewBuffer(CameraData* data)
{
	mRenderer->UpdatePerViewBuffer(*data);
}

void RenderServer::SetWireframe(bool value)
{
	if (value)
	{
		mRenderer->SetFlags(mRenderer->GetFlags() | WIRE_FRAME);
	}
	else
	{
		mRenderer->SetFlags(mRenderer->GetFlags() & ~WIRE_FRAME);
	}
}

void RenderServer::SetShowGBuffer(bool value)
{
	if (value)
	{
		mRenderer->SetFlags(mRenderer->GetFlags() | SHOW_GBUFFERS);
	}
	else
	{
		mRenderer->SetFlags(mRenderer->GetFlags() & ~SHOW_GBUFFERS);
	}
}

void RenderServer::SetShowBoundingBoxes(bool value)
{
	if (value)
	{
		mRenderer->SetFlags(mRenderer->GetFlags() | SHOW_BOUNDING_BOXES);
	}
	else
	{
		mRenderer->SetFlags(mRenderer->GetFlags() & ~SHOW_BOUNDING_BOXES);
	}
}

void RenderServer::SetShowIcons(bool value)
{
	if (value)
	{
		mRenderer->SetFlags(mRenderer->GetFlags() | SHOW_ICONS);
	}
	else
	{
		mRenderer->SetFlags(mRenderer->GetFlags() & ~SHOW_ICONS);
	}
}

bool RenderServer::GetShowIcons()
{
	return (mRenderer->GetFlags() & SHOW_ICONS) == SHOW_ICONS;
}
