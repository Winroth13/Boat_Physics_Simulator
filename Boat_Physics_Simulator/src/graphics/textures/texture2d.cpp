#include "graphics/textures/texture2d.h"
#include "stbimage/stb_image.h"
#include "core/logger.h"

Texture2D::Texture2D()
{
}

Texture2D::~Texture2D()
{
    mTexture->Release();
    mShaderResourceView->Release();
}

char* Texture2D::LoadImageData(
    const std::string& path,
    int& width,
    int& height,
    int& channels,
    int desiredChannels
)
{
    return (char*)stbi_load(
        path.c_str(),
        &width,
        &height,
        &channels,
        desiredChannels
    );
}

float Texture2D::GetAspect()
{
    return (float)GetWidth() / GetHeight();
}
