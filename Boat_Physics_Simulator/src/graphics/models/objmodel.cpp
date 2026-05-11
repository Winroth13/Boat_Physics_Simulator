#include "graphics/models/objmodel.h"
#include "graphics/shaders/vertexshader.h"
#include "graphics/meshes/mesh.h"
#include "graphics/textures/imagetexture2d.h"
#include "graphics/materials/material.h"

#include "core/logger.h"

#include <filesystem>
#include <unordered_map>

#include "tinyobjloader/tiny_obj_loader.h"

OBJModel::OBJModel(
	const std::string& path,
	std::shared_ptr<VertexShader> vertexShader,
	const bool isLeftHanded
)
{
	std::unordered_map<Vertex, size_t> uniqueVertices = {};
	std::vector<size_t> meshesIndicies;

	std::vector<std::vector<Vertex>> vertices;
	std::vector<std::vector<uint32_t>> indicies;
	std::vector<std::string> names;

	tinyobj::ObjReaderConfig config;
	config.mtl_search_path = "";

	tinyobj::ObjReader reader;

	if (!reader.ParseFromFile(path, config))
	{
		if (!reader.Error().empty())
		{
			Logger::Error("TinyOBJ failed to parse file: " + path + ", reason: " + reader.Error());
			// TODO: Maybe add fallback model here
			throw std::runtime_error("");
		}
	}

	if (!reader.Warning().empty())
	{
		Logger::Warn("TinyOBJ warning: " + reader.Warning());
	}

	auto& attrib = reader.GetAttrib();
	auto& shapes = reader.GetShapes();
	auto& objMaterials = reader.GetMaterials();

	for (size_t shapeIndex = 0; shapeIndex < shapes.size(); ++shapeIndex)
	{
		//uniqueVertices.clear();
		vertices.emplace_back(std::vector<Vertex>());
		indicies.emplace_back(std::vector<uint32_t>());

		auto& shape = shapes[shapeIndex];
		size_t nFaces = shape.mesh.num_face_vertices.size();

		names.emplace_back(shape.name);

		// Assume every face in the same shape shares the same material, maybe something to think about later.
		meshesIndicies.push_back(shape.mesh.material_ids[0]);

		size_t indexOffset = 0;
		for (size_t faceIndex = 0; faceIndex < nFaces; ++faceIndex)
		{
			if (shape.mesh.material_ids[0] != shape.mesh.material_ids[faceIndex])
			{
				Logger::Warn("Mesh " + shape.name + " has multiple materials");
			}

			size_t nVerticesInFace = size_t(shape.mesh.num_face_vertices[faceIndex]);
			for (size_t vertexIndex = 0; vertexIndex < nVerticesInFace; ++vertexIndex)
			{
				tinyobj::index_t idx = shape.mesh.indices[indexOffset + vertexIndex];
				tinyobj::real_t x = attrib.vertices[3 * size_t(idx.vertex_index) + 0];
				tinyobj::real_t y = attrib.vertices[3 * size_t(idx.vertex_index) + 1];
				tinyobj::real_t z = attrib.vertices[3 * size_t(idx.vertex_index) + 2];

				if (idx.normal_index < 0)
				{
					Logger::Error("Loading mesh has no normal data");
				}

				tinyobj::real_t nx = attrib.normals[3 * size_t(idx.normal_index) + 0];
				tinyobj::real_t ny = attrib.normals[3 * size_t(idx.normal_index) + 1];
				tinyobj::real_t nz = attrib.normals[3 * size_t(idx.normal_index) + 2];

				if (idx.texcoord_index < 0)
				{
					Logger::Error("Loading mesh has no uv data");
				}

				tinyobj::real_t tx = attrib.texcoords[2 * size_t(idx.texcoord_index) + 0];
				tinyobj::real_t ty = attrib.texcoords[2 * size_t(idx.texcoord_index) + 1];

				if (isLeftHanded)
				{
					ty = 1 - ty;
				}

				Vertex vertex =
				{
					{x, y, z},
					{nx, ny, nz},
					{tx, ty}
				};

				/* Vertex deduplication */
				if (uniqueVertices.count(vertex) == 0)
				{
					vertices[shapeIndex].push_back(vertex);
					uniqueVertices[vertex] = static_cast<uint32_t>(vertices[shapeIndex].size() - 1);
				}

				indicies[shapeIndex].push_back(static_cast<uint32_t>(uniqueVertices[vertex]));
			}
			indexOffset += nVerticesInFace;
		}
	}

	/* Calculate tangent */
	for (size_t i = 0; i < indicies.size(); ++i)
	{
		auto& meshIndicies = indicies[i];
		auto& meshVertices = vertices[i];

		for (size_t j = 0; j < meshIndicies.size(); j += 3)
		{
			Vertex& v0 = meshVertices[meshIndicies[j]];
			Vertex& v1 = meshVertices[meshIndicies[j + 1]];
			Vertex& v2 = meshVertices[meshIndicies[j + 2]];

			DirectX::XMVECTOR pos0 = DirectX::XMLoadFloat3(&v0.mPosition);
			DirectX::XMVECTOR pos1 = DirectX::XMLoadFloat3(&v1.mPosition);
			DirectX::XMVECTOR pos2 = DirectX::XMLoadFloat3(&v2.mPosition);

			DirectX::XMVECTOR uv0 = DirectX::XMLoadFloat2(&v0.mUV);
			DirectX::XMVECTOR uv1 = DirectX::XMLoadFloat2(&v1.mUV);
			DirectX::XMVECTOR uv2 = DirectX::XMLoadFloat2(&v2.mUV);

			DirectX::XMVECTOR edge0 = DirectX::XMVectorSubtract(pos1, pos0);
			DirectX::XMVECTOR edge1 = DirectX::XMVectorSubtract(pos2, pos0);

			DirectX::XMVECTOR deltaUV0 = DirectX::XMVectorSubtract(uv1, uv0);
			DirectX::XMVECTOR deltaUV1 = DirectX::XMVectorSubtract(uv2, uv0);

			DirectX::XMFLOAT3 edge0f;
			DirectX::XMFLOAT3 edge1f;

			DirectX::XMFLOAT2 deltaUV0f;
			DirectX::XMFLOAT2 deltaUV1f;

			DirectX::XMStoreFloat3(&edge0f, edge0);
			DirectX::XMStoreFloat3(&edge1f, edge1);

			DirectX::XMStoreFloat2(&deltaUV0f, deltaUV0);
			DirectX::XMStoreFloat2(&deltaUV1f, deltaUV1);

			// Note:
			// When we divide by 1.0f here we sometimes get infinity resulting in NaN on the GPU, which makes
			// all surfaces with normal maps become pitch black. Why are we even dividing by 1 here since
			// we normalize it later anyways?

			float f = 1.0f; /// (deltaUV0f.x * deltaUV1f.y - deltaUV1f.x * deltaUV0f.y);
			DirectX::XMFLOAT3 tangent;
			tangent.x = f * (deltaUV1f.y * edge0f.x - deltaUV0f.y * edge1f.x);
			tangent.y = f * (deltaUV1f.y * edge0f.y - deltaUV0f.y * edge1f.y);
			tangent.z = f * (deltaUV1f.y * edge0f.z - deltaUV0f.y * edge1f.z);

			v0.mTangent.x += tangent.x;
			v0.mTangent.y += tangent.y;
			v0.mTangent.z += tangent.z;

			v1.mTangent.x += tangent.x;
			v1.mTangent.y += tangent.y;
			v1.mTangent.z += tangent.z;

			v2.mTangent.x += tangent.x;
			v2.mTangent.y += tangent.y;
			v2.mTangent.z += tangent.z;
		}
	}

	/* Normalize tangents */
	for (auto& vertexArr : vertices)
	{
		for (auto& vertex : vertexArr)
		{
			DirectX::XMVECTOR tangent = DirectX::XMLoadFloat3(&vertex.mTangent);
			tangent = DirectX::XMVector3Normalize(tangent);
			DirectX::XMStoreFloat3(&vertex.mTangent, tangent);
		}
	}

	size_t materialCount = objMaterials.size();
	for (size_t materialIndex = 0; materialIndex < materialCount; ++materialIndex)
	{
		auto& objMaterial = objMaterials[materialIndex];

		std::shared_ptr<ImageTexture2D> diffuseTexture = nullptr;
		std::shared_ptr<ImageTexture2D> normalTexture = nullptr;
		std::shared_ptr<ImageTexture2D> displacementTexture = nullptr;
		std::shared_ptr<ImageTexture2D> alphaTexture = nullptr;

		std::string dir = std::filesystem::path(path)
			.parent_path()
			.string();

		/* Load Diffuse Texture	*/
		{
			std::string texturePath = dir + "/" + objMaterial.diffuse_texname;
			diffuseTexture = std::make_shared<ImageTexture2D>(texturePath);
		}

		std::shared_ptr<Material> material = std::make_shared<Material>(vertexShader, diffuseTexture);

		/* Load Normal Texture	*/
		{
			if (objMaterial.normal_texname.length() > 0)
			{
				std::string texturePath = dir + "/" + objMaterial.normal_texname;
				normalTexture = std::make_shared<ImageTexture2D>(texturePath);
				material->SetNormalMap(normalTexture);
			}
		}

		/* Load Displacement Texture */
		{
			if (objMaterial.displacement_texname.length() > 0)
			{
				std::string texturePath = dir + "/" + objMaterial.displacement_texname;
				displacementTexture = std::make_shared<ImageTexture2D>(texturePath);
				material->SetDisplacementMap(displacementTexture);
			}
		}

		/* Load Alpha Texture */
		{
			if (objMaterial.alpha_texname.length() > 0)
			{
				std::string texturePath = dir + "/" + objMaterial.alpha_texname;
				alphaTexture = std::make_shared<ImageTexture2D>(texturePath);
				material->SetAlphaMap(alphaTexture);
			}
		}

		material->SetAmbientCoefficient(objMaterial.ambient[0], objMaterial.ambient[1], objMaterial.ambient[2]);
		material->SetDiffuseCoefficient(objMaterial.diffuse[0], objMaterial.diffuse[1], objMaterial.diffuse[2]);
		material->SetSpecularCoefficient(objMaterial.specular[0], objMaterial.specular[1], objMaterial.specular[2]);

		float phongExponent = objMaterial.shininess;
		if (phongExponent == 0)
		{
			phongExponent = 1;
			material->SetSpecularCoefficient(0, 0, 0);
		}
		material->SetPhongExponent(phongExponent);
		material->SetName(objMaterial.name);

		AddMaterial(material);
	}

	size_t meshCount = vertices.size();
	for (size_t meshIndex = 0; meshIndex < meshCount; ++meshIndex)
	{
		std::shared_ptr<Mesh> mesh = std::make_shared<Mesh>();
		mesh->Create(vertices[meshIndex], indicies[meshIndex]);
		mesh->SetName(names[meshIndex]);

		AddMesh(mesh, meshesIndicies[meshIndex]);
	}

	Logger::Info(
		"Loaded obj model with " + std::to_string(meshCount) +
		" meshes and " + std::to_string(materialCount) + " materials: " + path
	);
}

OBJModel::~OBJModel()
{
}