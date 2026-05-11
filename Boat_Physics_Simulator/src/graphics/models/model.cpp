#include "graphics/models/model.h"
#include "core/renderer/renderserver.h"
#include "graphics/meshes/mesh.h"
#include "graphics/materials/material.h"
#include "core/logger.h"
#include "core/imguiflags.h"
#include "imgui/imgui.h"

#include <format>

Model::Model()
{
}

Model::Model(std::shared_ptr<Mesh> mesh, std::shared_ptr<Material> material)
{
	AddMesh(mesh);
	AddMaterial(material);
}

Model::~Model()
{
}

void Model::AddMesh(std::shared_ptr<Mesh> mesh, size_t materialIndex)
{
	mMeshes.emplace_back(mesh);
	mMaterialIndicies.push_back(materialIndex);
	mMeshVisibility.push_back(true);

	mVertices += mesh->GetNumVertices();
	mVisibleVertices += mesh->GetNumVertices();
	mIndicies += mesh->GetNumIndicies();
	mVisibleIndicies += mesh->GetNumIndicies();
}

void Model::AddMaterial(std::shared_ptr<Material> material)
{
	mMaterials.emplace_back(material);
}

std::shared_ptr<Mesh> Model::GetMeshByName(std::string name)
{
	for (auto& mesh : mMeshes)
	{
		if (mesh->GetName() == name)
		{
			return mesh;
		}
	}

	Logger::Warn("Failed to get mesh of name " + name);

	return nullptr;
}

std::shared_ptr<Material> Model::GetMaterialByName(std::string name)
{
	for (auto& material : mMaterials)
	{
		if (material->GetName() == name)
		{
			return material;
		}
	}

	Logger::Warn("Failed to get material of name " + name);

	return nullptr;
}

size_t Model::GetNumIndicies(size_t index)
{
	return mMeshes[index]->GetNumIndicies();
}

void Model::SetMeshVisibility(size_t index, const bool visible)
{
	auto& mesh = mMeshes[index];

	if (visible && mMeshVisibility[index] != true)
	{
		mVisibleVertices += mesh->GetNumVertices();
		mVisibleIndicies += mesh->GetNumIndicies();
	}
	else if (mMeshVisibility[index] != false)
	{
		mVisibleVertices -= mesh->GetNumVertices();
		mVisibleIndicies -= mesh->GetNumIndicies();
	}

	mMeshVisibility[index] = visible;
}

void Model::RenderImgui()
{
	if (ImGui::TreeNodeEx("Model", TREE_NODE_FLAGS))
	{
		ImGui::Text("Vertices: %d (%d)", mVertices, mVisibleVertices);
		ImGui::Text("Indicies: %d (%d)", mIndicies, mVisibleIndicies);

		std::string materialsHeader = std::format("Materials ({})", mMaterials.size());
		if (ImGui::TreeNodeEx(materialsHeader.c_str(), TREE_NODE_FLAGS))
		{
			for (auto& material : mMaterials)
			{
				if (ImGui::TreeNodeEx(material->GetName().c_str(), ImGuiTreeNodeFlags_DrawLinesToNodes | ImGuiTreeNodeFlags_Framed))
				{
					material->RenderImgui();
					ImGui::TreePop();
				}
			}
			ImGui::TreePop();
		}

		std::string meshesHeader = std::format("Meshes ({})", mMeshes.size());
		if (ImGui::TreeNodeEx(meshesHeader.c_str(), TREE_NODE_FLAGS))
		{
			ImGui::Text("Toggle Visibility");
			ImGui::SameLine();
			if (ImGui::Button("All"))
			{
				for (int i = 0; i < GetMeshCount(); ++i)
				{
					SetMeshVisibility(i, true);
				}
			}
			ImGui::SameLine();
			if (ImGui::Button("None"))
			{
				for (int i = 0; i < GetMeshCount(); ++i)
				{
					SetMeshVisibility(i, false);
				}
			}

			for (int i = 0; i < GetMeshCount(); ++i)
			{
				bool visible = IsMeshVisible(i);
				ImGui::PushID(i);
				if (ImGui::Checkbox("", &visible))
					SetMeshVisibility(i, visible);
				ImGui::PopID();

				ImGui::SameLine();

				if (ImGui::TreeNodeEx(mMeshes[i]->GetName().c_str(), ImGuiTreeNodeFlags_DrawLinesToNodes | ImGuiTreeNodeFlags_Framed))
				{
					mMeshes[i]->RenderImgui();
					ImGui::Text("Material: %s", mMaterials[mMaterialIndicies[i]]->GetName().c_str());
					ImGui::TreePop();
				}
			}
			ImGui::TreePop();
		}

		ImGui::TreePop();
	}
}