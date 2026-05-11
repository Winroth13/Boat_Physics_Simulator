#include <d3d11.h>

#include "graphics/shaders/vertexshader.h"
#include "core/renderer/renderer.h"
#include "core/logger.h"

VertexShader::VertexShader(const std::string& path)
    :mVertexShader(nullptr), Shader(path)
{
    LoadShaderData(path, mByteCode);

    HRESULT hr = Renderer::GetDevice()->CreateVertexShader(
        mByteCode.c_str(),
        mByteCode.length(),
        nullptr,
        &mVertexShader
    );

    if (FAILED(hr))
    {
        // TODO: maybe implement some fallback shader!
        Logger::Error(std::string("Failed to create vertex shader: ") + path);
        throw std::runtime_error("");
    }

    Logger::Info("Loaded vertex shader: " + path);
}

VertexShader::~VertexShader()
{
    mVertexShader->Release();
}

ID3D11VertexShader* VertexShader::GetShader()
{
    return mVertexShader;
}