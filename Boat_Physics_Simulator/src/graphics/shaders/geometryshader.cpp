#include "graphics/shaders/geometryshader.h"
#include "core/renderer/renderer.h"
#include "core/logger.h"

GeometryShader::GeometryShader(const std::string& path)
	:mGeometryShader(nullptr), Shader(path)
{
	std::string shaderByteCode;
	LoadShaderData(path, shaderByteCode);

	HRESULT hr = Renderer::GetDevice()->CreateGeometryShader(
		shaderByteCode.c_str(),
		shaderByteCode.length(),
		nullptr,
		&mGeometryShader
	);

	if (FAILED(hr))
	{
		// TODO: maybe implement some fallback shader!
		Logger::Error(std::string("Failed to create geometry shader: ") + path);
		throw std::runtime_error("");
	}

	Logger::Info("Loaded geometry shader: " + path);
}

GeometryShader::~GeometryShader()
{
	mGeometryShader->Release();
}

ID3D11GeometryShader* GeometryShader::GetShader()
{
	return mGeometryShader;
}