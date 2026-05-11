#include "graphics/shaders/computeshader.h"
#include "core/renderer/renderer.h"
#include "core/logger.h"

ComputeShader::ComputeShader(const std::string& path)
	:mComputeShader(nullptr), Shader(path)
{
	std::string shaderByteCode;
	LoadShaderData(path, shaderByteCode);

	HRESULT hr = Renderer::GetDevice()->CreateComputeShader(
		shaderByteCode.c_str(),
		shaderByteCode.length(),
		nullptr,
		&mComputeShader
	);

	if (FAILED(hr))
	{
		// TODO: maybe implement some fallback shader!
		Logger::Error(std::string("Failed to create compute shader: ") + path);
		throw std::runtime_error("");
	}

	Logger::Info("Loaded compute shader: " + path);
}

ComputeShader::~ComputeShader()
{
	mComputeShader->Release();
}

ID3D11ComputeShader* ComputeShader::GetShader()
{
	return mComputeShader;
}