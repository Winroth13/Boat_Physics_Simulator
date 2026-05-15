#pragma once
#include <memory>
#include <DirectXMath.h>
#include <DirectXCollision.h>

class Renderer;
class Model;
class Transform;
class Mesh;
class Material;
class Enviroment;
class CubemapTexture;
class Camera;
class ParticleSystem;
class Texture2D;


struct CameraData;
struct ID3D11DeviceContext;
struct AABB;

enum ConstantBufferType
{
	BUFFER_PER_FRAME = 0,
	BUFFER_PER_VIEW = 1,
	BUFFER_PER_OBJECT = 2,
	BUFFER_PER_MATERIAL = 3
};

class RenderServer
{
public:
	RenderServer() {}
	~RenderServer() {}

	bool Create(Renderer* renderer);

	void PushMesh(std::shared_ptr<Mesh> mesh, DirectX::XMMATRIX transform, bool isStatic = false);
	void PushMaterial(std::shared_ptr<Material> material, bool isStatic = false);

	void PushDirectionalLight(DirectX::XMFLOAT3 direction, DirectX::XMFLOAT3 color, float intensity);

	void PushPointLight(
		DirectX::XMFLOAT3 position,
		DirectX::XMFLOAT3 color,
		float attenuation,
		float intensity,
		bool shadows
	);

	void PushSpotLight(DirectX::XMFLOAT3 direction, DirectX::XMFLOAT3 position, DirectX::XMFLOAT3 color, float angle, float intensity, float attenuation);

	void PushCubemap(DirectX::XMFLOAT3 position, std::shared_ptr<CubemapTexture> cubemapTexture);

	void PushAABB(AABB aabb, DirectX::XMFLOAT3 color);
	void PushLine(DirectX::XMFLOAT3 start, DirectX::XMFLOAT3 end, DirectX::XMFLOAT3 color);
	void PushParticleSystem(std::shared_ptr<ParticleSystem> particleSystem, const DirectX::XMMATRIX transform);
	void PushSprite(
		std::shared_ptr<Texture2D> texture, 
		const DirectX::XMMATRIX transform, 
		const float scale, 
		const DirectX::XMFLOAT3 tint
	);

	void UpdateCamera(const DirectX::XMMATRIX viewProj, const DirectX::XMMATRIX view, const DirectX::XMFLOAT3 cameraPos);
	void UpdateFrustum(DirectX::BoundingFrustum frustum);
	void UpdateEnviroment(Enviroment& enviroment);
	void UpdatePerObject(DirectX::XMMATRIX world);

	void UpdatePerViewBuffer(CameraData* data);

	void SetWireframe(bool value);
	void SetShowGBuffer(bool value);
	void SetShowBoundingBoxes(bool value);
	void SetShowIcons(bool value);

	bool GetShowIcons();

private:
	Renderer* mRenderer = nullptr;
};