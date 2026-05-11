#include "graphics/shaders/domainshader.h"
#include "core/renderer/renderer.h"
#include "core/logger.h"

DomainShader::DomainShader(const std::string& path)
	:mDomainShader(nullptr), Shader(path)
{
	std::string shaderByteCode;
	LoadShaderData(path, shaderByteCode);

	HRESULT hr = Renderer::GetDevice()->CreateDomainShader(
		shaderByteCode.c_str(),
		shaderByteCode.length(),
		nullptr,
		&mDomainShader
	);

	if (FAILED(hr))
	{
		// TODO: maybe implement some fallback shader!
		Logger::Error(std::string("Failed to create domain shader: ") + path);
		throw std::runtime_error("");
	}

	Logger::Info("Loaded domain shader: " + path);
}

DomainShader::~DomainShader()
{
	mDomainShader->Release();
}

ID3D11DomainShader* DomainShader::GetShader()
{
	return mDomainShader;
}