#include "main.h"
#include "core/imguiflags.h"
#include "imgui/imgui.h"

#include "core/logger.h"

#include "graphics/textures/imagetexture2d.h"
#include "graphics/textures/cubemaptexture.h"

#include "graphics/models/objmodel.h"

#include "graphics/shaders/vertexshader.h"
#include "graphics/shaders/pixelshader.h"
#include "graphics/materials/material.h"

#include "core/entities/modelentity.h"
#include "core/entities/cameraentity.h"
#include "core/entities/enviromententity.h"
#include "core/entities/pointlightentity.h"
#include "core/entities/directionallightentity.h"
#include "core/entities/spotlightentity.h"
#include "core/entities/cubemapentity.h"
#include "core/entities/particleentity.h"

#include "graphics/particlesystem.h"

#include "imgui/ImGuizmo.h"

#include <memory>
#include <array>

#include "core/quadtree.h"

class TestApp : public App
{
public:
	float time = 0;

	std::unique_ptr<Entity>* inspectorEntity = nullptr;
	Entity* cameraEntity;
	Entity* spinningCube;
	POINT previousMousePos;

	std::vector<PointLightEntity*> firePointLights;

	void Initialize() override
	{
		auto skyCubemapTexture = std::make_shared<CubemapTexture>(
			std::array<std::string, 6>
		{
			"assets/skybox/posx.jpg",
				"assets/skybox/negx.jpg",
				"assets/skybox/posy.jpg",
				"assets/skybox/negy.jpg",
				"assets/skybox/posz.jpg",
				"assets/skybox/negz.jpg"
		}
		);

		auto vShader = std::make_shared<VertexShader>("resources/VertexShader.cso");

		auto sponza = std::make_shared<OBJModel>("assets/sponza/sponza_optimized.obj", vShader, true);
		std::shared_ptr<Material> sponzaBricks = sponza->GetMaterialByName("bricks");
		sponzaBricks->SetMaxTessFactor(50.0f);
		sponzaBricks->SetMaxTessDistance(15.0f);
		sponzaBricks->SetMinTessDistance(30.0f);
		auto& entity = mScene->CreateEntity<ModelEntity>(sponza);
		entity.SetName("Sponza");
		entity.transform.SetScale(0.070f, 0.070f, 0.070f);
		entity.transform.SetAngles(0, DirectX::XM_PI / 2, 0);
		entity.SetFlag(EntityFlags::STATIC, true);

		cameraEntity = &mScene->CreateEntity<CameraEntity>();
		cameraEntity->transform.SetPosition(0, 1, 0);

		auto& enviromentEntity = mScene->CreateEntity<EnviromentEntity>();
		int ambientDivisor = 1;
		enviromentEntity.SetAmbientColor(108.f / (255 * ambientDivisor), 150.f / (255 * ambientDivisor), 177.f / (255 * ambientDivisor));

		auto& sunEntity = mScene->CreateEntity<DirectionalLightEntity>();
		sunEntity.transform.SetAngles(DirectX::XMConvertToRadians(60), DirectX::XMConvertToRadians(-100), 0);
		sunEntity.SetIntensity(0.75f);
		sunEntity.SetVisible(true);

		auto& spotEntity = mScene->CreateEntity<SpotLightEntity>();
		spotEntity.SetColor({ 1.0, 0.0, 0.0 });
		spotEntity.transform.SetPosition(-2.5f, 5.679f, -3.201f);
		spotEntity.transform.SetAngles(
			DirectX::XMConvertToRadians(-23), 
			DirectX::XMConvertToRadians(90), 
			DirectX::XMConvertToRadians(0)
		);
		spotEntity.SetIntensity(20);

		auto& spotEntity2 = mScene->CreateEntity<SpotLightEntity>();
		spotEntity2.SetColor({ 1.0f, 1.0f, 1.0f });
		spotEntity2.transform.SetPosition(-0.975f, 7.691f, -3.625f);
		spotEntity2.transform.SetAngles(
			DirectX::XMConvertToRadians(-9.0f), 
			DirectX::XMConvertToRadians(39.0f), 
			0
		);
		spotEntity2.SetIntensity(3);

		auto& spotEntity3 = mScene->CreateEntity<SpotLightEntity>();
		spotEntity3.SetColor({ 1.0f, 0.0f, 1.0f });
		spotEntity3.transform.SetPosition(-0.053f, 0.638f, 12.0f);
		spotEntity3.transform.SetAngles(
			DirectX::XMConvertToRadians(-13.0f),
			DirectX::XMConvertToRadians(88.0f),
			0
		);
		spotEntity3.SetIntensity(24);

		auto& cubemapEntity = mScene->CreateEntity<CubemapEntity>(512);
		cubemapEntity.SetDynamic(true);

		auto cube = std::make_shared<OBJModel>("assets/cube/cube.obj", vShader, true);
		cube->GetMaterial(0)->SetCubemapTexture(cubemapEntity.GetCubemapTexture());

		auto sphere = std::make_shared<OBJModel>("assets/sphere/sphere.obj", vShader, true);
		sphere->GetMaterial(0)->SetCubemapTexture(cubemapEntity.GetCubemapTexture());
		sphere->GetMaterial(0)->SetReflectiveness(1);

		auto& cubeEntity = mScene->CreateEntity<ModelEntity>(cube);
		cubeEntity.SetName("Cube");
		cubeEntity.transform.SetPosition(0, 2, 0);
		cubeEntity.transform.SetScale(0.5, 0.5, 0.5);
		spinningCube = &cubeEntity;

		auto& sphereEntity = mScene->CreateEntity<ModelEntity>(sphere);
		sphereEntity.SetName("Sphere");
		sphereEntity.transform.SetPosition(-0.084f, 4.934f, -2.6f);

		cubemapEntity.Attach(&sphereEntity);
		cubeEntity.Attach(&sphereEntity);

		/* Fires */
		constexpr DirectX::XMFLOAT3 firePositions[4] =
		{
			{1.42f, 1.23f, 6.20f},
			{-2.20f, 1.23f, 6.20f},
			{1.42f, 1.23f, -4.87f},
			{-2.20f, 1.23f, -4.87f}
		};

		for (int i = 0; i < 4; ++i)
		{
			auto& fireEntity = mScene->CreateEntity<Entity>();
			fireEntity.SetName("Fire");
			fireEntity.transform.SetPosition(firePositions[i]);

			/* Create Fire Pointlight */
			{
				auto& pointLightEntity = mScene->CreateEntity<PointLightEntity>();
				pointLightEntity.transform.SetPosition(0, 0.5f, 0);
				pointLightEntity.SetName("Fire Light");
				pointLightEntity.Attach(&fireEntity);
				pointLightEntity.SetDynamicShadows(false);

				pointLightEntity.SetColor({ 100 / 255.0f, 40 / 255.0f, 0 / 255.0f });
				pointLightEntity.SetIntensity(0.7f);
				pointLightEntity.SetAttenuation(0.1f);
			}

			/* Create Smoke Particle */
			{
				auto& smokeParticleEntity = mScene->CreateEntity<ParticleEntity>(28);
				smokeParticleEntity.SetName("Smoke Particles");
				smokeParticleEntity.Attach(&fireEntity);
				smokeParticleEntity.transform.SetPosition(0.0f, 0.35f, 0.0f);

				auto particleSystem = smokeParticleEntity.GetParticleSystem();
				auto colorTexture = std::make_shared<ImageTexture2D>("assets/fire/smoke_color.jpg");
				auto alphaTexture = std::make_shared<ImageTexture2D>("assets/fire/smoke_mask.jpg");

				particleSystem->SetColorTexture(colorTexture);
				particleSystem->SetAlphaTexture(alphaTexture);
				particleSystem->SetAtlasWidth(13);
				particleSystem->SetAtlasHeight(1);
				particleSystem->SetAnimated(true);
				particleSystem->SetDesaturate(false);
				particleSystem->SetAdditive(false);

				particleSystem->SetLifeTime(5.43f);
				particleSystem->SetSpawnRadius(0.1f);
				particleSystem->SetStartScale(0.3f);
				particleSystem->SetEndScale(0.1f);
				particleSystem->SetVelocity(0.24f);

				particleSystem->SetStartTint(169 / 255.0f, 169 / 255.0f, 169 / 255.f);
				particleSystem->SetEndTint(54 / 255.0f, 54 / 255.0f, 54 / 255.f);
			}
			/* Create Fire Particle */
			{
				auto& fireParticleEntity = mScene->CreateEntity<ParticleEntity>(28);
				fireParticleEntity.SetName("Fire Particles");
				fireParticleEntity.Attach(&fireEntity);

				auto particleSystem = fireParticleEntity.GetParticleSystem();
				auto colorTexture = std::make_shared<ImageTexture2D>("assets/fire/fire_atlas.jpg");
				auto alphaTexture = std::make_shared<ImageTexture2D>("assets/fire/fire_mask_atlas.jpg");

				particleSystem->SetColorTexture(colorTexture);
				particleSystem->SetAlphaTexture(alphaTexture);
				particleSystem->SetAtlasWidth(3);
				particleSystem->SetAtlasHeight(3);
				particleSystem->SetAnimated(true);
				particleSystem->SetDesaturate(false);
				particleSystem->SetAdditive(true);

				particleSystem->SetLifeTime(1.2f);
				particleSystem->SetSpawnRadius(0.12f);
				particleSystem->SetStartScale(0.3f);
				particleSystem->SetEndScale(0.105f);
				particleSystem->SetDesaturatePower(1.5f);
				particleSystem->SetVelocity(0.45f);

				particleSystem->SetEndTint(0, 0, 0);
			}
		}

		for (auto& entity : mScene->GetEntities())
		{
			PointLightEntity* pointEntity = dynamic_cast<PointLightEntity*>(entity.get());

			if (pointEntity != nullptr)
			{
				firePointLights.emplace_back(pointEntity);
			}
		}
	};

	void Shutdown() override
	{
	};

	void Update(double delta) override
	{
		time += (float)delta;

		constexpr float AMPLITUDE = 0.3f;
		constexpr float FREQ = 10.0f;
		constexpr size_t OCTAVES = 3;

		int i = 0;

		for (auto& pointLight : firePointLights)
		{
			float ratio = 1.0f;
			float freq = FREQ;
			for (size_t octave = 0; octave < OCTAVES; octave++)
			{
				ratio *= (sin(time * freq + 10 * i)) / 2.0f;
				freq /= 2.0f;
			}

			pointLight->SetAttenuation(ratio * AMPLITUDE + 0.1f);
			i++;
		}

		spinningCube->transform.RotateY((3.14f / 2.0f) * (float)delta * 0.5f);

		/* Only handle input if window is focused */
		if (GetFocus() != NULL)
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

					if (inspectorEntity != nullptr && entity == *inspectorEntity)
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
						inspectorEntity = &entity;
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
				inspectorEntity->get()->RenderImgui();

				/* Draw Gizmos */
				{
					ImGuizmo::Enable(true);
					ImGuizmo::SetRect(0, 0, 1280, 640); // TODO: Do not hardcode screen dimensions
					ImGuizmo::AllowAxisFlip(true);
					ImGuizmo::SetOrthographic(true);

					auto& camera = mScene->GetCamera();
					Entity* attachEnt = inspectorEntity->get()->GetAttachEntity();

					DirectX::XMFLOAT4X4 view = camera.GetView4x4f();
					DirectX::XMFLOAT4X4 proj = camera.GetProj4x4f();

					Transform localTransform = inspectorEntity->get()->transform;

					if (inspectorEntity->get()->HasAttach())
					{
						localTransform.Translate(attachEnt->GetGlobalPosition());
						localTransform.Rotate(attachEnt->GetGlobalAngles());
						localTransform.Scale(attachEnt->GetGlobalScale());
					}

					DirectX::XMFLOAT4X4 localMat = localTransform.GetMatrixf();

					Transform& transform = inspectorEntity->get()->transform;

					float translate[3] = { -1, -1, -1 };
					float rotation[3] = { -1, -1, -1 };
					float scale[3] = { -1, -1, -1 };

					if (ImGuizmo::Manipulate(*view.m, *proj.m, mGizmoOperation, ImGuizmo::LOCAL, *localMat.m, NULL, NULL))
					{
						ImGuizmo::DecomposeMatrixToComponents(*localMat.m, translate, rotation, scale);

						switch (mGizmoOperation)
						{
						case ImGuizmo::TRANSLATE:
							if (inspectorEntity->get()->HasAttach())
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
							if (inspectorEntity->get()->HasAttach())
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
	props.title = "Windal Renderer";
	props.width = 1280;
	props.height = 640;

	return props;
}