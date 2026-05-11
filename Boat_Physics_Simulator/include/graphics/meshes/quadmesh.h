#pragma once
#include "graphics/meshes/mesh.h"

class QuadMesh : public Mesh
{
public:
	QuadMesh(float size = 1.0f, float uvScale = 1.0f);
	~QuadMesh();

};