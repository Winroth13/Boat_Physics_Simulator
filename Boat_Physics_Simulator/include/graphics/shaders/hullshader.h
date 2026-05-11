#pragma once
#include <d3d11.h>

#include "graphics/shaders/shader.h"
#include "core/renderer/renderserver.h"

class HullShader : public Shader
{
public:
	HullShader(const std::string& path);
	~HullShader() override;

	ID3D11HullShader* GetShader();

private:
	ID3D11HullShader* mHullShader;
};
