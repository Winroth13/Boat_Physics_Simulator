#include "graphics/shaders/hullshader.h"
#include "core/renderer/renderer.h"
#include "core/logger.h"

HullShader::HullShader(const std::string& path)
	:mHullShader(nullptr), Shader(path)
{
	std::string shaderByteCode;
	LoadShaderData(path, shaderByteCode);

	HRESULT hr = Renderer::GetDevice()->CreateHullShader(
		shaderByteCode.c_str(),
		shaderByteCode.length(),
		nullptr,
		&mHullShader
	);

	if (FAILED(hr))
	{
		// TODO: maybe implement some fallback shader!
		Logger::Error(std::string("Failed to create hull shader: ") + path);
		throw std::runtime_error("");
	}

	Logger::Info("Loaded hull shader: " + path);
}

HullShader::~HullShader()
{
	mHullShader->Release();
}

ID3D11HullShader* HullShader::GetShader()
{
	return mHullShader;
}