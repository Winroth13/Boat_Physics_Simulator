#pragma once
#include <vector>
#include <string>
#include "core/renderer/renderserver.h"
#include "math/aabb.h"

struct ID3D11Buffer;
struct D3D11_INPUT_ELEMENT_DESC;

struct Vertex
{
	DirectX::XMFLOAT3 mPosition;
	DirectX::XMFLOAT3 mNormal;
	DirectX::XMFLOAT2 mUV;
	DirectX::XMFLOAT3 mTangent = { 0, 0, 0 };

	Vertex(
		const DirectX::XMFLOAT3 position,
		const DirectX::XMFLOAT3 normal,
		const DirectX::XMFLOAT2 uv
	) : mPosition(position), mNormal(normal), mUV(uv)
	{
	}
};

bool operator== (const Vertex& v1, const Vertex& v2);

//
namespace std {
	template <>
	struct hash<Vertex>
	{
		size_t operator()(const Vertex& v) const
		{
			size_t seed = 0;
			std::hash<float> hasher;
			auto combine = [&](float f)
				{
					seed ^= hasher(f) + (seed << 6) + (seed >> 2) + 0x9e3779b9;
				};

			combine(v.mPosition.x);
			combine(v.mPosition.y);
			combine(v.mPosition.z);

			combine(v.mNormal.x);
			combine(v.mNormal.y);
			combine(v.mNormal.z);

			combine(v.mUV.x);
			combine(v.mUV.y);

			return seed;
		}
	};
}

class Mesh
{
public:
	Mesh();
	~Mesh();

	bool Create(
		const std::vector<Vertex>& vertices,
		const std::vector<uint32_t>& indicies
	);

	void RenderImgui();

	const size_t GetNumIndicies() { return mNumIndicies; }
	const size_t GetNumVertices() { return mNumVertices; }

	const AABB& GetBounds() { return mBounds; }

	ID3D11Buffer* GetVertexBuffer() { return mVertexBuffer; }
	ID3D11Buffer* GetIndexBuffer() { return mIndexBuffer; }

	void SetName(const std::string& name) { mName = name; }
	const std::string& GetName() { return mName; }

private:
	size_t mNumVertices = 0;
	size_t mNumIndicies = 0;
	AABB mBounds;

	std::string mName;

	ID3D11Buffer* mVertexBuffer = nullptr;
	ID3D11Buffer* mIndexBuffer = nullptr;
};