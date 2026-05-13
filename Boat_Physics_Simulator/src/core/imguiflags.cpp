#include "core/imguiflags.h"
#include "imgui/imgui.h"

void DragPercentage(const std::string& name, float& val)
{
    float percentage = val * 100.0f;
    if (ImGui::DragFloat(name.c_str(), &percentage, 0.01f, 0, 100, "%.2f %%"))
    {
        val = percentage / 100.0f;
    }
}