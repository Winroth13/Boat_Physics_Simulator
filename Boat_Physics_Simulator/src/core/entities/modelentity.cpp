#include "core/entities/modelentity.h"
#include "graphics/models/model.h"
#include "graphics/meshes/mesh.h"

#include "core/renderer/renderserver.h"
#include "core/logger.h"

#include "math/mathfunctions.h"

#include "imgui/imgui.h"

#include <d3d11.h>

ModelEntity::ModelEntity(std::shared_ptr<Model> model)
	: mModel(model), Entity("Model")
{
}

ModelEntity::~ModelEntity() {}

void ModelEntity::BeginSelf(RenderServer& renderServer)
{
	/* Push static geometry on scene begin */
	if (HasFlag(EntityFlags::STATIC))
	{
		for (size_t meshIndex = 0; meshIndex < mModel->GetMeshCount(); ++meshIndex)
		{
			renderServer.PushMesh(mModel->GetMesh(meshIndex), GetGlobalTransform(), true);
			renderServer.PushMaterial(mModel->GetMaterial(meshIndex), true);
		}
	}
}

void ModelEntity::UpdateSelf(double delta)
{
	//transform.RotateY((3.14f / 8) * static_cast<float>(delta));
	//transform.RotateX((3.14f / 8) * static_cast<float>(delta));
	//transform.RotateZ((3.14f / 8) * static_cast<float>(delta));
}

void ModelEntity::RenderSelf(RenderServer& renderServer)
{
	if (!HasFlag(EntityFlags::STATIC))
	{
		for (size_t meshIndex = 0; meshIndex < mModel->GetMeshCount(); ++meshIndex)
		{
			if (mModel->IsMeshVisible(meshIndex))
			{
				renderServer.PushMesh(mModel->GetMesh(meshIndex), GetGlobalTransform());
				renderServer.PushMaterial(mModel->GetMaterial(meshIndex));
			}
		}
	}
}

void ModelEntity::RenderImguiSelf()
{
	mModel->RenderImgui();
}