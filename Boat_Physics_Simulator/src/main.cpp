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
#include "core/entities/spotlightentity.h"

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
	float time;

	Entity* inspectorEntity = nullptr;

	Entity* camera;
	Entity* ocean;
	BoatEntity* boat;
	Entity* mattias;

	POINT previousMousePos;

	void Initialize() override
	{
		auto skyboxCubemap = std::make_shared<CubemapTexture>(
			std::array<std::string, 6>
		{
			"assets/skybox/day/day_px.png",
			"assets/skybox/day/day_nx.png",
			"assets/skybox/day/day_py.png",
			"assets/skybox/day/day_ny.png",
			"assets/skybox/day/day_pz.png",
			"assets/skybox/day/day_nz.png"
		}
		);

		auto vShader = std::make_shared<VertexShader>("resources/VertexShader.cso");

		auto& gameEntity = mScene->CreateEntity<Entity>();
		gameEntity.SetName("Game");

		auto& enviromentEntity = mScene->CreateEntity<EnviromentEntity>();
		int ambientDivisor = 1;
		enviromentEntity.SetAmbientColor(108.f / (255 * ambientDivisor), 150.f / (255 * ambientDivisor), 177.f / (255 * ambientDivisor));

		auto& sunEntity = mScene->CreateEntity<DirectionalLightEntity>();
		sunEntity.transform.SetAngles(DirectX::XMConvertToRadians(60), DirectX::XMConvertToRadians(-100), 0);
		sunEntity.SetIntensity(0.75f);
		sunEntity.SetVisible(true);

		auto& boatEntity = mScene->CreateEntity<BoatEntity>();
		boatEntity.transform.SetPosition(0, 0.5f, 0);
		inspectorEntity = &boatEntity;
		boat = &boatEntity;
		boatEntity.Attach(&gameEntity);
		boatEntity.SetGameEntity(&gameEntity);
		boatEntity.SetVisible(false);

		auto& rootEntity = mScene->CreateEntity<Entity>();
		rootEntity.Attach(&boatEntity);
		rootEntity.transform.SetPosition(0, 0.28f, 0);
		rootEntity.SetName("Root");
		boatEntity.SetRootEntity(&rootEntity);

		auto boatModel = std::make_shared<OBJModel>("assets/motorboat/motorboat.obj", vShader, true);

		auto& boatModelEntity = mScene->CreateEntity<ModelEntity>(boatModel);
		boatModelEntity.Attach(&rootEntity);
		boatModelEntity.SetName("Boat Model");
		boatModelEntity.transform.SetPosition(0, 0, 0);
		boatModelEntity.transform.SetScale(1, 1, 1);
		boatEntity.SetBoatModelEntity(&boatModelEntity);

		auto boatMaterial = boatModelEntity.GetModel()->GetMaterial(0);
		boatMaterial->SetCubemapTexture(skyboxCubemap);
		boatMaterial->SetReflectiveness(0.1f);

		auto cubeModel = std::make_shared<OBJModel>("assets/cube/cube.obj", vShader, true);

		auto cubeMaterial = cubeModel->GetMaterial(0);
		cubeMaterial->SetCubemapTexture(skyboxCubemap);
		cubeMaterial->SetReflectiveness(0.0f);

		auto& cubeEntity = mScene->CreateEntity<ModelEntity>(cubeModel);
		cubeEntity.Attach(&rootEntity);
		cubeEntity.transform.SetPosition(-0.375f, 0.3f, 0.361f);
		cubeEntity.transform.SetAngles(
			DirectX::XMConvertToRadians(32.0f),
			DirectX::XMConvertToRadians(0.0f),
			DirectX::XMConvertToRadians(0.0f)
		);
		cubeEntity.transform.SetScale(0.2f, 0.2f, 0.2f);
		cubeEntity.SetName("Amalgam");

		auto& motorHingeEntity = mScene->CreateEntity<Entity>();
		motorHingeEntity.Attach(&rootEntity);
		motorHingeEntity.SetName("Motor Hinge");
		motorHingeEntity.transform.SetPosition(0.0f, 0.1f, -1.9f);
		boatEntity.SetMotorHingeEntity(&motorHingeEntity);

		auto boatEngineModel = std::make_shared<OBJModel>("assets/motorboat/engine.obj", vShader, true);
		auto& motorModelEntity = mScene->CreateEntity<ModelEntity>(boatEngineModel);
		motorModelEntity.Attach(&motorHingeEntity);
		motorModelEntity.SetName("Motor Model");
		motorModelEntity.transform.SetPosition(0, 0, 0);
		motorModelEntity.transform.SetScale(1, 1, 1);

		auto engineMaterial = boatEngineModel->GetMaterial(0);
		engineMaterial->SetCubemapTexture(skyboxCubemap);
		engineMaterial->SetReflectiveness(0.3f);

		auto& propellerEntity = mScene->CreateEntity<Entity>();
		propellerEntity.Attach(&motorHingeEntity);
		propellerEntity.SetName("Propeller");
		propellerEntity.transform.SetPosition(0.0f, -1.0f, -0.7f);
		boatEntity.SetPropellerEntity(&propellerEntity);

		/* Ocean */
		{
			auto vShaderWater = std::make_shared<VertexShader>("resources/WaterVertexShader.cso");

			auto oceanModel = std::make_shared<OBJModel>("assets/ocean/ocean.obj", vShaderWater);
			std::shared_ptr<Material> oceanMaterial = oceanModel->GetMaterialByName("Ocean");
			oceanMaterial->SetMaxTessFactor(40.0f);
			oceanMaterial->SetMinTessDistance(101.0f);
			oceanMaterial->SetMaxTessDistance(100.0f);
			oceanMaterial->SetDispStrength(0.3f);

			oceanMaterial->SetCubemapTexture(skyboxCubemap);
			oceanMaterial->SetReflectiveness(0.2f);

			auto& oceanEntity = mScene->CreateEntity<ModelEntity>(oceanModel);
			oceanEntity.transform.SetScale(100.0f, 1.0f, 100.0f);
			oceanEntity.SetName("Ocean");
			ocean = &oceanEntity;
			oceanEntity.Attach(&gameEntity);
		}

		auto& cameraEntity = mScene->CreateEntity<CameraEntity>();
		cameraEntity.transform.SetPitch(DirectX::XMConvertToRadians(18));
		cameraEntity.transform.SetPosition(
			DirectX::XMVectorAdd(
				DirectX::XMVECTOR{ 0, 1, 0 },
				DirectX::XMVectorScale(cameraEntity.transform.GetForwardDir(), -4)
			)
		);
		camera = &cameraEntity;
		boatEntity.SetCameraEntity(&cameraEntity);
		camera->Attach(boat);
		boat->SetVisible(true);
		boat->Unpause();
	};

	void Shutdown() override
	{
	};

	void Update(double delta) override
	{
		time += static_cast<float>(delta);

		/* Only handle input if window is focused */
		if (GetFocus() != NULL)
		{
			/* Camera Movement	*/
			constexpr float TURN_SPEED = 2.6f;

			POINT newMousePos;

			GetCursorPos(&newMousePos);

			bool cameraRotated = false;
			/* Rotation */
			if (GetAsyncKeyState(VK_LEFT) & 0x8000)
			{
				camera->transform.RotateY(TURN_SPEED * (float)delta);
				cameraRotated = true;
			}
			if (GetAsyncKeyState(VK_RIGHT) & 0x8000)
			{
				camera->transform.RotateY(-TURN_SPEED * (float)delta);
				cameraRotated = true;
			}
			if (GetAsyncKeyState(VK_UP) & 0x8000)
			{
				camera->transform.RotateX(TURN_SPEED * (float)delta);
				cameraRotated = true;
			}
			if (GetAsyncKeyState(VK_DOWN) & 0x8000)
			{
				camera->transform.RotateX(-TURN_SPEED * (float)delta);
				cameraRotated = true;
			}
			if (GetAsyncKeyState(MK_RBUTTON) & 0x8000)
			{
				float dx = 0.25f * DirectX::XMConvertToRadians(static_cast<float>(newMousePos.x - previousMousePos.x));
				float dy = 0.25f * DirectX::XMConvertToRadians(static_cast<float>(newMousePos.y - previousMousePos.y));

				camera->transform.RotateX(TURN_SPEED * dy);
				camera->transform.RotateY(TURN_SPEED * dx);

				cameraRotated = true;
			}

			//DirectX::XMFLOAT3 angles = camera->transform.GetAngles3f();
			//angles.x = 0.0f;
			//camera->transform.SetAngles(angles);
			//camera->transform.RotateX(-boat->transform.GetAngles3f().x);

			if (cameraRotated)
			{
				camera->transform.SetPosition(
					DirectX::XMVectorAdd(
						DirectX::XMVECTOR{ 0, 1, 0 },
						DirectX::XMVectorScale(camera->transform.GetForwardDir(), -4)
					)
				);
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

		/* Make water follow the boat */
		DirectX::XMFLOAT3 boatPos = boat->transform.GetPosition3f();
		DirectX::XMFLOAT3 oceanPos = ocean->transform.GetPosition3f();
		ocean->transform.SetPosition(boatPos.x, oceanPos.y, boatPos.z);
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

				if (false)
				{
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

	bool mShowHierarchy = false;
	bool mShowInspector = true;
	bool mShowDiagnostics = false;
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