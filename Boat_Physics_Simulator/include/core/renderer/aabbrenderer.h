#pragma once
#include <vector>
#include <memory>
#include <DirectXMath.h>

struct ID3D11Buffer;
struct ID3D11InputLayout;
struct ID3D11DeviceContext;

class VertexShader;
class PixelShader;

struct AABBBuffer
{
	DirectX::XMMATRIX transform;
	DirectX::XMFLOAT3 color;
	float pad0;
};

struct AABBData
{
	DirectX::XMFLOAT3 origin;
	DirectX::XMFLOAT3 size;
	DirectX::XMFLOAT3 color;
};

struct AABBVertex
{
	DirectX::XMFLOAT3 position;
};

class AABBRenderer
{
public:
	AABBRenderer();
	~AABBRenderer();

	bool Create();
	void Render(ID3D11DeviceContext* ctx, const std::vector<AABBData>& data);

private:
	ID3D11InputLayout* mInputLayout = nullptr;
	ID3D11Buffer* mVertexBuffer = nullptr;
	ID3D11Buffer* mIndexBuffer = nullptr;
	ID3D11Buffer* mCBuffer = nullptr;

	std::unique_ptr<VertexShader> mVertexShader = nullptr;
	std::unique_ptr<PixelShader> mPixelShader = nullptr;
};