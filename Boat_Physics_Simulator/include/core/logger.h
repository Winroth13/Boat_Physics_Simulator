#pragma once
#include <iostream>
#include <string>
#include <format>

class Logger
{
public:
	static void Info(const std::string& text);
	static void Error(const std::string& text);
	static void Warn(const std::string& text);
};