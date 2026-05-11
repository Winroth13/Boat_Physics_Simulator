#pragma once
#include <string>
#include <fstream>
#include <iostream>
#include <d3d11.h>

#include "core/renderer/renderserver.h"

class Shader
{
public:
	Shader(const std::string& path);
	virtual ~Shader() = default;

	const std::string& GetPath() { return mPath; }

protected:
	std::string mPath;

	bool LoadShaderData(const std::string& path, std::string& data);
};