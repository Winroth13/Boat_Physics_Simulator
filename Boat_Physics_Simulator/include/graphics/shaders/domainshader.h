#pragma once
#include <d3d11.h>

#include "graphics/shaders/shader.h"
#include "core/renderer/renderserver.h"

class DomainShader : public Shader
{
public:
	DomainShader(const std::string& path);
	~DomainShader() override;

	ID3D11DomainShader* GetShader();

private:
	ID3D11DomainShader* mDomainShader;
};
