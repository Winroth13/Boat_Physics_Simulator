#include "core/entities/enviromententity.h"
#include "core/scene.h"

#include "imgui/imgui.h"
#include "core/imguiflags.h"

EnviromentEntity::EnviromentEntity()  : Entity("Enviroment") {}

EnviromentEntity::~EnviromentEntity() {}

void EnviromentEntity::SetAmbientColor(float r, float g, float b)
{
    GetScene().GetEnviroment().SetAmbientColor(r, g, b);
}

void EnviromentEntity::UpdateSelf(double delta)
{
}

void EnviromentEntity::RenderSelf(RenderServer& renderServer)
{
    renderServer.UpdateEnviroment(GetScene().GetEnviroment());
}

void EnviromentEntity::RenderImguiSelf()
{
    if (ImGui::TreeNodeEx("Enviroment", TREE_NODE_FLAGS))
    {
        auto& enviroment = GetScene().GetEnviroment();
        ImGui::ColorEdit3("Ambient Color", &enviroment.GetAmbientColorRef().x);
        ImGui::Checkbox("Blinn Phong", &enviroment.GetUseBlinnPhong());
        ImGui::TreePop();
    }
}