#pragma once
#include <d3d11.h>

#include "graphics/shaders/shader.h"
#include "core/renderer/renderserver.h"

class GeometryShader : public Shader
{
public:
	GeometryShader(const std::string& path);
	~GeometryShader() override;

	ID3D11GeometryShader* GetShader();

private:
	ID3D11GeometryShader* mGeometryShader;
};
