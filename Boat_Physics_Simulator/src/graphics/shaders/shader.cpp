#include "graphics/shaders/shader.h"
#include "core/logger.h"

Shader::Shader(const std::string& path) : mPath(path) {}

bool Shader::LoadShaderData(const std::string& path, std::string& data)
{
	std::ifstream reader;
	reader.open(path, std::ios::binary | std::ios::ate);

	if (!reader.is_open())
	{
		Logger::Error("Could not open shader file: " + path);
		return false;
	}

	reader.seekg(0, std::ios::end);
	data.reserve(static_cast<unsigned int>(reader.tellg()));
	reader.seekg(0, std::ios::beg);

	data.assign(
		std::istreambuf_iterator<char>(reader),
		std::istreambuf_iterator<char>()
	);
	return true;
};