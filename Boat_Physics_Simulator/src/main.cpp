#include "main.h"
#include "core/imguiflags.h"
#include "imgui/imgui.h"

#include "core/logger.h"

#include "graphics/textures/imagetexture2d.h"
#include "graphics/textures/cubemaptexture.h"

#include "graphics/models/objmodel.h"
#include "graphics/meshes/quadmesh.h"

#include "graphics/shaders/vertexshader.h"
#include "graphics/shaders/pixelshader.h"
#include "graphics/materials/material.h"

#include "core/entities/modelentity.h"
#include "core/entities/cameraentity.h"
#include "core/entities/enviromententity.h"
#include "core/entities/directionallightentity.h"

#include "core/entities/boatentity.h"

#include "graphics/particlesystem.h"

#include "imgui/ImGuizmo.h"

#include <memory>
#include <array>

#include "core/quadtree.h"

extern "C" {
    __declspec(dllexport) DWORD NvOptimusEnablement = 1;
    __declspec(dllexport) DWORD AmdPowerXpressRequestHighPerformance = 1;
}

class TestApp : public App
{
public:
	Entity* inspectorEntity = nullptr;

	Entity* cameraEntity;

	POINT previousMousePos;

	void Initialize() override
	{
		auto vShader = std::make_shared<VertexShader>("resources/VertexShader.cso");

		auto& enviromentEntity = mScene->CreateEntity<EnviromentEntity>();
		int ambientDivisor = 1;
		enviromentEntity.SetAmbientColor(108.f / (255 * ambientDivisor), 150.f / (255 * ambientDivisor), 177.f / (255 * ambientDivisor));

		auto& sunEntity = mScene->CreateEntity<DirectionalLightEntity>();
		sunEntity.transform.SetAngles(DirectX::XMConvertToRadians(60), DirectX::XMConvertToRadians(-100), 0);
		sunEntity.SetIntensity(0.75f);
		sunEntity.SetVisible(true);

		auto& boatEntity = mScene->CreateEntity<BoatEntity>();
		boatEntity.transform.SetPosition(0, 0.050f, 0);
		inspectorEntity = &boatEntity;

		auto boatModel = std::make_shared<OBJModel>("assets/motorboat/MotorBoat.obj", vShader);

		auto& boatModelEntity = mScene->CreateEntity<ModelEntity>(boatModel);
		boatModelEntity.Attach(&boatEntity);
		boatModelEntity.SetName("Boat Model");
		boatModelEntity.transform.SetPosition(0, 0, 0);
		boatModelEntity.transform.SetScale(1, 1, 1);

		auto cubeModel = std::make_shared<OBJModel>("assets/cube/cube.obj", vShader);
		mScene->CreateEntity<ModelEntity>(cubeModel).Attach(&boatEntity);

		auto& motorHingeEntity = mScene->CreateEntity<Entity>();
		motorHingeEntity.Attach(&boatEntity);
		motorHingeEntity.SetName("Motor Hinge");
		motorHingeEntity.transform.SetPosition(0, 0.6f, -3.3);

		boatEntity.SetMotorHingeEntity(&motorHingeEntity);

		auto boatEngineModel = std::make_shared<OBJModel>("assets/motorboat/MotorBoatEngine.obj", vShader);
		auto& motorModelEntity = mScene->CreateEntity<ModelEntity>(boatEngineModel);
		motorModelEntity.Attach(&motorHingeEntity);
		motorModelEntity.SetName("Motor Model");
		motorModelEntity.transform.SetPosition(0, 0, 0);
		motorModelEntity.transform.SetScale(1, 1, 1);

		/* Ocean */
		{
			auto oceanMesh = std::make_shared<QuadMesh>(1024.0f, 128.0f);
			oceanMesh->SetName("Ocean Mesh");
			auto oceanTexture = std::make_shared<ImageTexture2D>("assets/ocean/water_diffuse.jpg");

			auto oceanMaterial = std::make_shared<Material>(vShader, oceanTexture);
			oceanMaterial->SetName("Ocean Material");

			auto oceanModel = std::make_shared<Model>(oceanMesh, oceanMaterial);

			auto& oceanModelEntity = mScene->CreateEntity<ModelEntity>(oceanModel);
			oceanModelEntity.SetName("Ocean");
		}

		auto& cameraEntity = mScene->CreateEntity<CameraEntity>();
		cameraEntity.transform.SetPosition(0, 5.8, -8.7);
		cameraEntity.transform.SetPitch(DirectX::XMConvertToRadians(18));
		cameraEntity.Attach(&boatEntity);
	};

	void Shutdown() override
	{
	};

	void Update(double delta) override
	{
		/* Only handle input if window is focused */
		if (GetFocus() != NULL && false)
		{
			/* Camera Movement	*/
			constexpr float SPEED = 3.0f;
			constexpr float TURN_SPEED = 2.6f;

			POINT newMousePos;

			GetCursorPos(&newMousePos);

			float speed = SPEED;
			if (GetAsyncKeyState(VK_LSHIFT))
			{
				speed *= 3.0f;
			}

			/* Movement */
			if (GetAsyncKeyState('W') & 0x8000)
				cameraEntity->transform.MoveForward(speed * (float)delta);
			if (GetAsyncKeyState('S') & 0x8000)
				cameraEntity->transform.MoveForward(-speed * (float)delta);
			if (GetAsyncKeyState('A') & 0x8000)
				cameraEntity->transform.MoveRight(-speed * (float)delta);
			if (GetAsyncKeyState('D') & 0x8000)
				cameraEntity->transform.MoveRight(speed * (float)delta);
			if (GetAsyncKeyState(VK_SPACE) & 0x8000)
				cameraEntity->transform.MoveUp(speed * (float)delta);
			if (GetAsyncKeyState(VK_LCONTROL) & 0x8000)
				cameraEntity->transform.MoveUp(-speed * (float)delta);

			/* Rotation */
			if (GetAsyncKeyState(VK_LEFT) & 0x8000)
				cameraEntity->transform.RotateY(-TURN_SPEED * (float)delta);
			if (GetAsyncKeyState(VK_RIGHT) & 0x8000)
				cameraEntity->transform.RotateY(TURN_SPEED * (float)delta);
			if (GetAsyncKeyState(VK_UP) & 0x8000)
				cameraEntity->transform.RotateX(-TURN_SPEED * (float)delta);
			if (GetAsyncKeyState(VK_DOWN) & 0x8000)
				cameraEntity->transform.RotateX(TURN_SPEED * (float)delta);
			if (GetAsyncKeyState(MK_RBUTTON) & 0x8000)
			{
				float dx = 0.25f * DirectX::XMConvertToRadians(static_cast<float>(newMousePos.x - previousMousePos.x));
				float dy = 0.25f * DirectX::XMConvertToRadians(static_cast<float>(newMousePos.y - previousMousePos.y));

				cameraEntity->transform.RotateX(TURN_SPEED * dy);
				cameraEntity->transform.RotateY(TURN_SPEED * dx);

				/* Clamp pitch */
				if (cameraEntity->transform.GetAngles3f().x > DirectX::XM_PIDIV2)
				{
					cameraEntity->transform.SetPitch(DirectX::XM_PIDIV2);
				}
				else if (cameraEntity->transform.GetAngles3f().x < -DirectX::XM_PIDIV2)
				{
					cameraEntity->transform.SetPitch(-DirectX::XM_PIDIV2);
				}
			}

			/* Toggle Gizmo Operation */
			if (GetAsyncKeyState('T') & 0x8000)
				mGizmoOperation = ImGuizmo::TRANSLATE;
			if (GetAsyncKeyState('R') & 0x8000)
				mGizmoOperation = ImGuizmo::SCALE;

			if (GetAsyncKeyState(VK_ESCAPE) & 0x8000)
			{
				inspectorEntity = nullptr;
			}

			previousMousePos = newMousePos;
		}
	};

	void Render(RenderServer& renderServer) override
	{
	};

	void ImguiRender(RenderServer& renderServer) override
	{
		if (ImGui::BeginMainMenuBar())
		{
			if (ImGui::BeginMenu("File"))
			{
				if (ImGui::MenuItem("Exit"))
				{
					Quit();
				}
				ImGui::EndMenu();
			}

			if (ImGui::BeginMenu("View"))
			{
				ImGui::Checkbox("Scene Hierarchy", &mShowHierarchy);
				ImGui::Checkbox("Inspector", &mShowInspector);
				ImGui::Checkbox("Diagnostics", &mShowDiagnostics);
				ImGui::EndMenu();
			}

			if (ImGui::BeginMenu("Debug"))
			{
				ImGui::Checkbox("Lock Frustum", &mScene->GetLockFrustum());

				bool showIcons = renderServer.GetShowIcons();
				if (ImGui::Checkbox("Show Icons", &showIcons))
				{
					renderServer.SetShowIcons(showIcons);
				}

				if (ImGui::BeginMenu("Viewport"))
				{
					bool changed = false;

					if (ImGui::RadioButton("Default", mViewportDebugMode == 0))
					{
						mViewportDebugMode = 0;
						changed = true;
					}

					if (ImGui::RadioButton("Wireframe", mViewportDebugMode == 1))
					{
						mViewportDebugMode = 1;
						changed = true;
					}

					if (ImGui::RadioButton("G-Buffers", mViewportDebugMode == 2))
					{
						mViewportDebugMode = 2;
						changed = true;
					}

					if (ImGui::RadioButton("Bounding Boxes", mViewportDebugMode == 3))
					{
						mViewportDebugMode = 3;
						changed = true;
					}

					if (changed)
					{
						renderServer.SetWireframe(mViewportDebugMode == 1);
						renderServer.SetShowGBuffer(mViewportDebugMode == 2);
						renderServer.SetShowBoundingBoxes(mViewportDebugMode == 3);
					}

					ImGui::EndMenu();
				}

				ImGui::EndMenu();
			}

			ImGui::EndMainMenuBar();
		}

		if (mShowHierarchy)
		{
			ImGui::Begin("Scene Hierarchy");

			if (ImGui::TreeNodeEx("Scene", ImGuiTreeNodeFlags_DefaultOpen))
			{
				for (auto& entity : mScene->GetEntities())
				{
					ImGuiTreeNodeFlags flags =
						ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen;

					if (inspectorEntity != nullptr && entity.get() == inspectorEntity)
					{
						flags |= ImGuiTreeNodeFlags_Selected;
					}

					if (!entity->IsVisible())
					{
						ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(150 / 255.0f, 150 / 255.0f, 150 / 255.0f, 1.0f));
					}

					ImGui::TreeNodeEx(entity->GetName().c_str(), flags);

					if (!entity->IsVisible())
					{
						ImGui::PopStyleColor(1);
					}

					if (ImGui::IsItemClicked())
					{
						inspectorEntity = entity.get();
					}
				}
				ImGui::TreePop();
			}

			ImGui::End();
		}

		if (mShowInspector)
		{
			ImGui::Begin("Inspector");
			if (inspectorEntity != nullptr)
			{
				inspectorEntity->RenderImgui();

				/* Draw Gizmos */
				{
					ImGuizmo::Enable(true);
					ImGuizmo::SetRect(0, 0, 1280, 640); // TODO: Do not hardcode screen dimensions
					ImGuizmo::AllowAxisFlip(true);
					ImGuizmo::SetOrthographic(true);

					auto& camera = mScene->GetCamera();
					Entity* attachEnt = inspectorEntity->GetAttachEntity();

					DirectX::XMFLOAT4X4 view = camera.GetView4x4f();
					DirectX::XMFLOAT4X4 proj = camera.GetProj4x4f();

					DirectX::XMFLOAT4X4 localMat;
					DirectX::XMStoreFloat4x4(&localMat, inspectorEntity->GetGlobalTransform());

					Transform& transform = inspectorEntity->transform;

					float translate[3] = { -1, -1, -1 };
					float rotation[3] = { -1, -1, -1 };
					float scale[3] = { -1, -1, -1 };

					if (ImGuizmo::Manipulate(*view.m, *proj.m, mGizmoOperation, ImGuizmo::LOCAL, *localMat.m, NULL, NULL))
					{
						ImGuizmo::DecomposeMatrixToComponents(*localMat.m, translate, rotation, scale);

						switch (mGizmoOperation)
						{
						case ImGuizmo::TRANSLATE:
							if (inspectorEntity->HasAttach())
							{
								DirectX::XMFLOAT3 invAttachPos = attachEnt->GetGlobalPosition();
								invAttachPos.x = -invAttachPos.x;
								invAttachPos.y = -invAttachPos.y;
								invAttachPos.z = -invAttachPos.z;
								translate[0] += invAttachPos.x;
								translate[1] += invAttachPos.y;
								translate[2] += invAttachPos.z;
							}

							transform.SetPosition(translate[0], translate[1], translate[2]);
							break;

						case ImGuizmo::SCALE:
							if (inspectorEntity->HasAttach())
							{
								DirectX::XMFLOAT3 invAttachScale = attachEnt->GetGlobalScale();
								invAttachScale.x = -invAttachScale.x;
								invAttachScale.y = -invAttachScale.y;
								invAttachScale.z = -invAttachScale.z;
								scale[0] += invAttachScale.x;
								scale[1] += invAttachScale.y;
								scale[2] += invAttachScale.z;
							}
							transform.SetScale(scale[0], scale[1], scale[2]);
							break;

						default:
							break;
						}
					}
				}
			}
			ImGui::End();
		}

		if (mShowDiagnostics)
		{
			ImGui::Begin("Diagnostics");
			ImGui::Text("%.2f FPS (%.1f ms)", ImGui::GetIO().Framerate, ImGui::GetIO().DeltaTime * 1000);
			ImGui::Text("Resolution: %.0fx%.0f", ImGui::GetIO().DisplaySize.x, ImGui::GetIO().DisplaySize.y);
			ImGui::End();
		}
	};

private:
	uint32_t mViewportDebugMode = 0;

	ImGuizmo::OPERATION mGizmoOperation = ImGuizmo::TRANSLATE;

	bool mShowHierarchy = true;
	bool mShowInspector = true;
	bool mShowDiagnostics = true;
};

App* CreateApp()
{
	return new TestApp();
}

WindowProps CreateWindowProperties()
{
	WindowProps props;
	props.title = "Boat Physics Simulator";
	props.width = 1280;
	props.height = 640;

	return props;
}