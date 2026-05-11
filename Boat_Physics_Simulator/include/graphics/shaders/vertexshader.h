#pragma once
#include <d3d11.h>
#include "graphics/shaders/shader.h"

class VertexShader : public Shader
{
public:
	VertexShader(const std::string& path);
	~VertexShader() override;

	ID3D11VertexShader* GetShader();

	const std::string& GetByteCode() { return mByteCode; }

private:
	std::string mByteCode;
	ID3D11VertexShader* mVertexShader;
};