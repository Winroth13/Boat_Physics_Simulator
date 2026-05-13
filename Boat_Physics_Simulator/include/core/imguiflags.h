#pragma once
#define TREE_NODE_FLAGS ImGuiTreeNodeFlags_DrawLinesToNodes | ImGuiTreeNodeFlags_Framed | ImGuiTreeNodeFlags_DefaultOpen

#include <string>

void DragPercentage(const std::string& name, float& val);