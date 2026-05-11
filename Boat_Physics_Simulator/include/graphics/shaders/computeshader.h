#pragma once
#include <d3d11.h>

#include "graphics/shaders/shader.h"
#include "core/renderer/renderserver.h"

class ComputeShader : public Shader
{
public:
	ComputeShader(const std::string& path);
	~ComputeShader() override;

	ID3D11ComputeShader* GetShader();

private:
	ID3D11ComputeShader* mComputeShader;
};
