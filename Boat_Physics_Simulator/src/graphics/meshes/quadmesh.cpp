#include "graphics/meshes/quadmesh.h"
#include "core/logger.h"

QuadMesh::QuadMesh(float size, float uvScale)
{
	std::vector<Vertex> vertices = {
		{{ -size, 0.0f, -size }, { 0.0f, 1.0f, 0.0f }, { 0.0f, 1.0f * uvScale }},
		{{ -size, 0.0f, size }, { 0.0f, 1.0f, 0.0f }, { 0.0f, 0.0f }},
		{{ size, 0.0f, size }, { 0.0f, 1.0f, 0.0f }, { 1.0f * uvScale, 0.0f }},
		{{ size, 0.0f, -size }, { 0.0f, 1.0f, 0.0f }, { 1.0f * uvScale, 1.0f * uvScale }}
	};

	std::vector<uint32_t> indices = { 0, 1, 2, 3, 0, 2 };

	if (Create(vertices, indices) == false)
	{
		Logger::Error("Failed to create quad mesh");
		Create({}, {});
	}
}