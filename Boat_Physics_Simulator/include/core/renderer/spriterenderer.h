#pragma once

#include <DirectXMath.h>
#include <memory>
#include <vector>

class Texture2D;
class PixelShader;
class VertexShader;
class ComputeShader;
class GeometryShader;
class RenderServer;

struct ID3D11DeviceContext;
struct ID3D11InputLayout;

#define MAX_SPRITES 64

struct SpriteData
{
	std::shared_ptr<Texture2D> texture;
	DirectX::XMMATRIX transform;
	float scale;
	DirectX::XMFLOAT3 tint;
};

struct SpriteBuffer
{
	DirectX::XMFLOAT3 position = { 0, 0, 0 };
	float scale = 1.0f;
	DirectX::XMFLOAT3 tint = { 1.0f, 0.0f, 0.0f };
};

class SpriteRenderer
{
public:
	SpriteRenderer();
	~SpriteRenderer();

	bool Create();
	void Render(
		ID3D11DeviceContext* ctx,
		RenderServer& renderServer,
		std::vector<SpriteData>& data
	);

private:
	std::unique_ptr<VertexShader> mVertexShader;
	std::unique_ptr<PixelShader> mPixelShader;
	std::unique_ptr<GeometryShader> mGeometryShader;

	ID3D11InputLayout* mInputLayout = nullptr;
};