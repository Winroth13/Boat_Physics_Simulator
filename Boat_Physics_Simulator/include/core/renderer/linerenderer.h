#pragma once

#include <vector>
#include <memory>
#include <DirectXMath.h>

struct ID3D11Buffer;
struct ID3D11InputLayout;
struct ID3D11DeviceContext;

class VertexShader;
class PixelShader;

struct LineData
{
	DirectX::XMFLOAT3 start;
	DirectX::XMFLOAT3 end;
	DirectX::XMFLOAT3 color;
};

struct LineVertex
{
	DirectX::XMFLOAT3 position;
	DirectX::XMFLOAT3 color;
};

class LineRenderer
{
public:
	LineRenderer();
	~LineRenderer();

	bool Create();
	void Render(ID3D11DeviceContext* ctx, const std::vector<LineData>& data);

private:
	ID3D11InputLayout* mInputLayout = nullptr;
	ID3D11Buffer* mVertexBuffer = nullptr;

	std::unique_ptr<VertexShader> mVertexShader = nullptr;
	std::unique_ptr<PixelShader> mPixelShader = nullptr;
};