#pragma once
#include <d3d11.h>

#include "graphics/shaders/shader.h"
#include "core/renderer/renderserver.h"

class PixelShader : public Shader
{
public:
	PixelShader(const std::string& path);
	~PixelShader() override;

	ID3D11PixelShader* GetShader();

private:
	ID3D11PixelShader* mPixelShader;
};