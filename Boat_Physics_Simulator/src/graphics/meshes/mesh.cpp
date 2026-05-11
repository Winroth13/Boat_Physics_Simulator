#include "graphics/meshes/mesh.h"
#include "core/renderer/renderer.h"
#include "core/logger.h"

#include <d3d11.h>
#include "core/imguiflags.h"
#include "imgui/imgui.h"

bool operator== (const Vertex& v1, const Vertex& v2)
{
	return
		(
			v1.mPosition.x == v2.mPosition.x &&
			v1.mPosition.y == v2.mPosition.y &&
			v1.mPosition.z == v2.mPosition.z &&

			v1.mNormal.x == v2.mNormal.x &&
			v1.mNormal.y == v2.mNormal.y &&
			v1.mNormal.z == v2.mNormal.z &&

			v1.mUV.x == v2.mUV.x &&
			v1.mUV.y == v2.mUV.y
			);
}

Mesh::Mesh()
{
}

Mesh::~Mesh()
{
	mVertexBuffer->Release();
	mIndexBuffer->Release();
}

bool Mesh::Create(
	const std::vector<Vertex>& vertices,
	const std::vector<uint32_t>& indicies)
{
	if (mVertexBuffer != nullptr && mIndexBuffer != nullptr)
	{
		Logger::Warn("Trying to create on an already existing mesh");
		return false;
	}

	D3D11_BUFFER_DESC vertexBufferDesc = {};
	vertexBufferDesc.ByteWidth = UINT(sizeof(Vertex) * vertices.size());
	vertexBufferDesc.Usage = D3D11_USAGE_IMMUTABLE;
	vertexBufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	vertexBufferDesc.CPUAccessFlags = 0;
	vertexBufferDesc.MiscFlags = 0;
	vertexBufferDesc.StructureByteStride = 0;

	D3D11_SUBRESOURCE_DATA vertexData;
	vertexData.pSysMem = vertices.data();
	vertexData.SysMemPitch = 0;
	vertexData.SysMemSlicePitch = 0;

	HRESULT hr = Renderer::GetDevice()->CreateBuffer(
		&vertexBufferDesc,
		&vertexData,
		&mVertexBuffer
	);

	if (FAILED(hr))
	{
		Logger::Error("Failed to create vertex buffer");
		return false;
	}

	D3D11_BUFFER_DESC indexbufferDesc;
	indexbufferDesc.ByteWidth = UINT(sizeof(uint32_t) * indicies.size());
	indexbufferDesc.Usage = D3D11_USAGE_IMMUTABLE;
	indexbufferDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;
	indexbufferDesc.CPUAccessFlags = 0;
	indexbufferDesc.MiscFlags = 0;

	D3D11_SUBRESOURCE_DATA indexbufferData;
	indexbufferData.pSysMem = indicies.data();
	indexbufferData.SysMemPitch = 0;
	indexbufferData.SysMemSlicePitch = 0;

	hr = Renderer::GetDevice()->CreateBuffer(
		&indexbufferDesc,
		&indexbufferData,
		&mIndexBuffer
	);

	if (FAILED(hr))
	{
		Logger::Error("Failed to create index buffer");
		mVertexBuffer->Release();
		return false;
	}

	mNumVertices = vertices.size();
	mNumIndicies = indicies.size();

	mBounds.mMin = vertices[0].mPosition;
	mBounds.mMax = mBounds.mMin;
	for (int i = 1; i < vertices.size(); ++i)
	{
		mBounds.Expand(vertices[i].mPosition);
	}

	return true;
}

void Mesh::RenderImgui()
{
	ImGui::Text("Vertices: %d", mNumVertices);
	ImGui::Text("Indicies: %d", mNumIndicies);
}