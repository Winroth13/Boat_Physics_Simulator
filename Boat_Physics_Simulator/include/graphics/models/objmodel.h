#pragma once
#include "graphics/models/model.h"
#include <string>

class VertexShader;

class OBJModel : public Model
{
public:
	OBJModel(
		const std::string& path,
		std::shared_ptr<VertexShader> vertexShader,
		const bool isLeftHanded = false
	);

	~OBJModel();
};