#pragma once
#include <d3d11.h>

#include "core/window.h"
#include "core/renderer/renderserver.h"
#include "core/renderer/aabbrenderer.h"
#include "core/renderer/linerenderer.h"
#include "core/renderer/particlerenderer.h"
#include "core/renderer/spriterenderer.h"

#include "graphics/materials/material.h"
#include "graphics/meshes/mesh.h"
#include "graphics/particlesystem.h"

#include "graphics/shaders/vertexshader.h"
#include "graphics/shaders/pixelshader.h"
#include "graphics/shaders/computeshader.h"
#include "graphics/shaders/domainshader.h"
#include "graphics/shaders/hullshader.h"

#include "graphics/shadowmap.h"
#include "graphics/omnidirectionalshadowmap.h"
#include "graphics/gbuffers.h"

#include "math/transform.h"
#include "core/quadtree.h"

#include <DirectXMath.h>
#include <DirectXCollision.h>

#include <vector>
#include <array>
#include <unordered_map>

enum RenderFlags
{
	WIRE_FRAME = 1,
	SHOW_GBUFFERS = 2,
	USE_BLINN_PHONG = 4,
	SHOW_BOUNDING_BOXES = 8,
	SHOW_ICONS = 16
};

enum class ShaderType
{
	VERTEX,
	PIXEL,
	COMPUTE,
	DOMAIN_SHADER,
	HULL,
	GEOMETRY
};

struct PerFrameBuffer
{
	DirectX::XMFLOAT3 ambientColor;
	uint32_t numDirectionalLights;
	uint32_t numPointLights;
	uint32_t numSpotLights;
	uint32_t flags;
	float elapsedTime;
	std::array<uint32_t, 2> screenDimensions;
	DirectX::XMFLOAT2 pad0;
	DirectX::XMFLOAT3 sceneCameraPos;
	float pad1;
};

struct PerViewBuffer
{
	DirectX::XMMATRIX viewProj;
	DirectX::XMMATRIX view;
	DirectX::XMFLOAT3 cameraPos;
    float pad0;
};

struct PerObject
{
	DirectX::XMMATRIX world;
	DirectX::XMMATRIX worldInverseTranspose;
};

struct PerMaterial
{
	DirectX::XMFLOAT3 ambientCoefficient;
	float phongExponent;
	DirectX::XMFLOAT3 diffuseCoefficient;
	float reflectiveness;
	DirectX::XMFLOAT3 specularCoefficient;
	uint32_t materialFlags;
	float maxTessFactor;
	float maxTessDistance;
	float minTessDistance;
	float dispStrength;
};

struct MaterialIndexBuffer
{
	uint32_t materialIndex;
	DirectX::XMFLOAT3 pad0;
};

struct DirectionalLightData
{
	DirectX::XMMATRIX viewProj;
	DirectX::XMFLOAT3 direction;
	float intensity;
	DirectX::XMFLOAT3 color;
	float pad0;
};

struct PointLightData
{
	DirectX::XMFLOAT3 position;
	float attenuation;
	DirectX::XMFLOAT3 color;
	float intensity;
	bool updateShadows;
};

struct PointLightBuffer
{
	DirectX::XMFLOAT3 position;
	float attenuation;
	DirectX::XMFLOAT3 color;
	float intensity;
};

struct SpotLightData
{
	DirectX::XMMATRIX viewProj;
	DirectX::XMFLOAT3 position;
	float attenuation;
	DirectX::XMFLOAT3 color;
	float intensity;
	DirectX::XMFLOAT3 direction;
	float angle;
};

struct MaterialData
{
	std::shared_ptr<Material> material;
};

struct GeometryData
{
	std::shared_ptr<Mesh> mesh;
	DirectX::XMMATRIX transform;
};

struct EnviromentData
{
	DirectX::XMFLOAT3 ambientColor;
	bool useBlinnPhong;
};

struct CameraData
{
	DirectX::XMMATRIX viewProj;
	DirectX::XMMATRIX view;
	DirectX::XMFLOAT3 pos;
};

struct FrustumData
{
	DirectX::BoundingFrustum frustum;
};

struct CubemapData
{
	std::shared_ptr<CubemapTexture> cubemapTexture;
	DirectX::XMFLOAT3 position;
};

// TODO: Extract more sub-renderers to make the class more manageable

class Renderer
{
public:
	Renderer();
	~Renderer();

	bool Create(DirectX::XMFLOAT4 clearColor, Window* window);
	void Shutdown();

	void BeginRender();
	void Render();

	void BeginForward(ID3D11RenderTargetView* renderTarget = nullptr, ID3D11DepthStencilView* depthStencilView = nullptr);
	void RenderForward();
	void EndForward();

	void PresentRender();

	static ID3D11Device* GetDevice() { return Renderer::sDevice; }
	static ID3D11DeviceContext* GetContext() { return Renderer::mImmediateContext; }

	RenderServer& GetRenderServer() { return Renderer::mRenderServer; }
	static RenderServer& GetRenderServerStatic() { return Renderer::mRenderServer; }

	void UpdatePerFrameBuffer(
		const DirectX::XMFLOAT3 ambientColor,
		const uint32_t numDirectionalLights,
		const uint32_t numPointLights,
		const uint32_t numSpotLights,
		const uint32_t flags,
		const std::array<uint32_t, 2> screenDimensions
	);

	void PushGeometryData(const GeometryData& geometryData, bool isStatic = false);
	void PushMaterialData(const MaterialData& materialData, bool isStatic = false);

	void PushDirectionalLightData(const DirectionalLightData& directionalLightData);
	void PushPointLightData(const PointLightData& pointLightData);
	void PushSpotLightData(const SpotLightData& spotLightData);

	void PushCubemapData(const CubemapData& cubemapData);

	void PushAABBData(const AABBData& aabbData);
	void PushLineData(const LineData& lineData);
	void PushParticleSystemData(const ParticleSystemData& particleSystemData);
	void PushSpriteData(const SpriteData& spriteData);

	void SetEnviromentData(const EnviromentData& enviromentData);
	void SetSceneCamera(const CameraData& cameraData);
	void SetCullingFrustum(const FrustumData& frustum);

	const uint16_t GetFlags() { return mNewFlags; }
	void SetFlags(const uint16_t flags) { mNewFlags = flags; }

	void BakeStaticGeometry();

	void UpdatePerObjectBuffer(const DirectX::XMMATRIX world);
	void UpdatePerViewBuffer(const CameraData& cameraData);

private:
	void RenderShadowMaps();
	void RenderCubeMaps();
	void RenderDeferred(ID3D11UnorderedAccessView* backBuffer, GBuffers& buffers, uint16_t flags, CameraData camera, FrustumData frustum);
	void RenderBB();

	bool IsGeometryVisible(GeometryData& geometryData, const DirectX::BoundingFrustum& frustum);

	void RenderShadowMeshAndMaterial(GeometryData& geometryData, MaterialData& materialData);

	void RenderDeferredMeshAndMaterial(
		GeometryData& geometryData,
		MaterialData& materialData,
		std::unordered_map<std::shared_ptr<Material>, uint32_t>& materialsMap,
		std::vector<PerMaterial>& perMaterials
	);

	void UpdatePerMaterialBuffer(std::shared_ptr<Material> material);

	/* Bind Functions*/
	void BindGBuffers(GBuffers& buffers);
	void UnbindGBuffers();

	void BindPerFrameBuffer(ShaderType shaderType);
	void BindPerViewBuffer(ShaderType shaderType);
	void BindPerMaterialBuffer(ShaderType shaderType);

	void BindMaterialSRV(std::shared_ptr<Material> material, uint32_t index);
	void BindMesh(std::shared_ptr<Mesh> mesh);

	void BindVertexShader(std::shared_ptr<VertexShader> vertexShader);
	void BindVertexShader(const std::unique_ptr<VertexShader>& vertexShader);

	void BindPixelShader(std::shared_ptr<PixelShader> pixelShader);
	void BindPixelShader(const std::unique_ptr<PixelShader>& pixelShader);

	void BindDomainShader(std::shared_ptr<DomainShader> domainShader);
	void BindDomainShader(const std::unique_ptr<DomainShader>& domainShader);

	void BindHullShader(std::shared_ptr<HullShader> hullShader);
	void BindHullShader(const std::unique_ptr<HullShader>& hullShader);

	void BindTexture2D(std::shared_ptr<Texture2D> texture2d, UINT slot, ShaderType type = ShaderType::PIXEL);

	void BindDirectionalLights(ShaderType shaderType);
	void BindPointLights(ShaderType shaderType);
	void BindSpotLights(ShaderType shaderType);

	/* Unbind Functions */
	void UnbindMaterial();
	void UnbindMesh();
	void UnbindVertexShader();
	void UnbindPixelShader();
	void UnbindDomainShader();
	void UnbindHullShader();
	void UnbindTexture2D(UINT slot);

	/* Update Buffers */
	void UpdateMaterialIndexBuffer(uint32_t index);

	void ClearFrameData();

	/* Renderers */
	AABBRenderer mAABBRenderer;
	LineRenderer mLineRenderer;
	ParticleRenderer mParticleRenderer;
	SpriteRenderer mSpriteRenderer;

	/* Static Data */
	std::vector<GeometryData> mStaticGeometryData;
	std::vector<MaterialData> mStaticMaterialData;
	QuadTree<size_t> mStaticGeometryTree;

	/* Dynamic Model Data */
	std::vector<GeometryData> mGeometryData;
	std::vector<MaterialData> mMaterialData;

	EnviromentData mEnviromentData;
	CameraData mSceneCamera;	// TODO: Maybe this should have a better name...
	FrustumData mSceneFrustum;	// TODO: Maybe this should have a better name...

	/* GBuffers */
	GBuffers mGBuffers;

	/* Lights */
	std::vector<DirectionalLightData> mDirectionalLightsData;
	std::vector<PointLightData> mPointLightsData;
	std::vector<SpotLightData> mSpotLightsData;

	/* Shadow Mapping */
	std::unique_ptr<PixelShader> mShadowMapPixelShader;
	std::unique_ptr<PixelShader> mShadowMapLinearPixelShader;

	ShadowMap mDirectionalLightsShadowMap;
	OmnidirectionalShadowMap mPointLightsShadowMap;
	ShadowMap mSpotLightsShadowMap;
	ID3D11SamplerState* mShadowMapSampler = nullptr;

	/* Particle System */
	std::vector<ParticleSystemData> mParticleSystemsData;

	/* Sprites */
	std::vector<SpriteData> mSpritesData;

	/* Cubemaps */
	std::vector<CubemapData> mCubemapsData;

	/* Debug Drawing */
	std::vector<AABBData> mAABBData;
	std::vector<LineData> mLineData;

    std::unique_ptr<CubemapTexture> mSkyboxTexture;

	DirectX::XMFLOAT4 mClearColor;
	Window* mWindow;

	static ID3D11Device* sDevice;
	static ID3D11DeviceContext* mImmediateContext;

	std::unique_ptr<ComputeShader> mLightingComputeShader;
	std::unique_ptr<PixelShader> mDeferredPixelShader;

	std::unique_ptr<HullShader> mTessellationHullShader;
	std::unique_ptr<DomainShader> mDisplacementDomainShader;

	IDXGISwapChain* mSwapChain;
	ID3D11RenderTargetView* mBackBufferRenderTargetView;
	ID3D11UnorderedAccessView* mBackBufferUAV;

	static RenderServer mRenderServer;

	ID3D11SamplerState* mDefaultSampler;

	ID3D11Buffer* mPerFrameBuffer;
	ID3D11Buffer* mPerViewBuffer;
	ID3D11Buffer* mPerObjectBuffer;

	ID3D11Buffer* mMaterialIndexBuffer;

	ID3D11Buffer* mDirectionalLightsBuffer;
	ID3D11ShaderResourceView* mDirectionalLightsSRV;

	ID3D11Buffer* mPointLightsBuffer;
	ID3D11ShaderResourceView* mPointLightsSRV;

	ID3D11Buffer* mSpotLightsBuffer;
	ID3D11ShaderResourceView* mSpotLightsSRV;

	ID3D11Buffer* mMaterialsBuffer;
	ID3D11ShaderResourceView* mMaterialsSRV;

	ID3D11Buffer* mPerMaterialBuffer;
	ID3D11ShaderResourceView* mPerMaterialSRV;

	D3D11_RASTERIZER_DESC mRasterizerDesc = {};

	uint16_t mFlags = 0;
	uint16_t mNewFlags = 0;

	float mElapsedTime = 0;
};