#include "core/entities/particleentity.h"
#include "graphics/particlesystem.h"
#include "graphics/textures/texture2d.h"

#include "core/imguiflags.h"
#include "imgui/imgui.h"

#include "core/logger.h"
#include "graphics/textures/imagetexture2d.h"

ParticleEntity::ParticleEntity(uint32_t count)
	: Entity("Particle")
{
	mParticleSystem = std::make_shared<ParticleSystem>(count);
	mIcon = std::make_shared<ImageTexture2D>("assets/sprites/particle.png");
}

ParticleEntity::~ParticleEntity()
{
}

void ParticleEntity::UpdateSelf(double delta)
{
	mParticleSystem->Update(delta);
}

void ParticleEntity::RenderSelf(RenderServer& renderServer)
{
	if (mParticleSystem->GetColorTexture() && mParticleSystem->GetAlphaTexture())
		renderServer.PushParticleSystem(mParticleSystem, GetGlobalTransform());

	renderServer.PushSprite(mIcon, GetGlobalTransform(), 0.2f, { 0.2f, 0.2f, 0.5f });
}

void ParticleEntity::RenderImguiSelf()
{
	if (ImGui::TreeNodeEx("Particle System", TREE_NODE_FLAGS))
	{
		float offset = ImGui::GetFrameHeight() + ImGui::GetStyle().ItemInnerSpacing.x;
		ImGui::Indent(offset);

		if (ImGui::Button("Reset"))
		{
			mParticleSystem->Reset();
		}

		if (ImGui::TreeNodeEx("General", ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_Framed))
		{
			/* Count */
			int count = (uint32_t)mParticleSystem->GetCount();
			if (ImGui::DragInt("Count", &count, 0.1f, 1, INT32_MAX))
			{
				mParticleSystem->SetCount(count);
				mParticleSystem->Reset();
			}

			/* Lifetime */
			{
				float lifeTime = mParticleSystem->GetLifeTime();
				if (ImGui::DragFloat("Lifetime", &lifeTime, 0.01f, 0, FLT_MAX))
				{
					mParticleSystem->SetLifeTime(lifeTime);
					mParticleSystem->Reset();
				}
			}

			/* Spawn Radius */
			{
				float spawnRadius = mParticleSystem->GetSpawnRadius();
				if (ImGui::DragFloat("Spawn Radius", &spawnRadius, 0.01f, 0, FLT_MAX))
				{
					mParticleSystem->SetSpawnRadius(spawnRadius);
				}
			}

			/* Velocity */
			{
				float velocity = mParticleSystem->GetVelocity();
				if (ImGui::DragFloat("Velocity", &velocity, 0.01f, 0, FLT_MAX))
				{
					mParticleSystem->SetVelocity(velocity);
				}
			}

			/* Start Tint */
			{
				DirectX::XMFLOAT3 startTint = mParticleSystem->GetStartTint();
				if (ImGui::ColorEdit3("Start Tint", &startTint.x))
				{
					mParticleSystem->SetStartTint(startTint);
				}
			}

			/* End Tint */
			{
				DirectX::XMFLOAT3 endTint = mParticleSystem->GetEndTint();
				if (ImGui::ColorEdit3("End Tint", &endTint.x))
				{
					mParticleSystem->SetEndTint(endTint);
				}
			}

			/* Start Scale */
			{
				float startScale = mParticleSystem->GetStartScale();
				if (ImGui::DragFloat("Start Scale", &startScale, 0.01f, 0, FLT_MAX))
				{
					mParticleSystem->SetStartScale(startScale);
				}
			}

			/* End Scale */
			{
				float endScale = mParticleSystem->GetEndScale();
				if (ImGui::DragFloat("End Scale", &endScale, 0.01f, 0, FLT_MAX))
				{
					mParticleSystem->SetEndScale(endScale);
				}
			}

			/* Additive */
			{
				bool isAdditive = mParticleSystem->IsAdditive();
				if (ImGui::Checkbox("Additive", &isAdditive))
				{
					mParticleSystem->SetAdditive(isAdditive);
				}
			}

			ImGui::TreePop();
		}

		ImGui::Unindent(offset);

		/* Animation */
		{
			bool isAnimated = mParticleSystem->IsAnimated();
			if (ImGui::Checkbox("##Animated", &isAnimated))
			{
				mParticleSystem->SetAnimated(isAnimated);
			}

			ImGui::SameLine();

			ImGuiTreeNodeFlags flags = isAnimated ? ImGuiTreeNodeFlags_DefaultOpen : ImGuiTreeNodeFlags_Leaf;

			if (ImGui::TreeNodeEx("Animated", flags | ImGuiTreeNodeFlags_Framed))
			{
				if (isAnimated)
				{
					/* Animation Speed */
					{
						float animationSpeed = mParticleSystem->GetAnimationSpeed();
						if (ImGui::DragFloat("Speed", &animationSpeed, 0.01f, 0, FLT_MAX))
						{
							mParticleSystem->SetAnimationSpeed(animationSpeed);
						}
					}

					/* Atlas Size */
					{
						float atlasWidth = mParticleSystem->GetAtlasWidth();
						float atlasHeight = mParticleSystem->GetAtlasHeight();
						int atlasSize[2] = { (int)atlasWidth, (int)atlasHeight };
						if (ImGui::DragInt2("Atlas Size", &atlasSize[0], 0.05f, 1, INT32_MAX))
						{
							mParticleSystem->SetAtlasWidth((float)atlasSize[0]);
							mParticleSystem->SetAtlasHeight((float)atlasSize[1]);
						}
					}
				}
				ImGui::TreePop();
			}
		}

		/* Desaturate */
		{
			bool isDesaturate = mParticleSystem->IsDesaturate();
			if (ImGui::Checkbox("##Desaturate", &isDesaturate))
			{
				mParticleSystem->SetDesaturate(isDesaturate);
			}

			ImGui::SameLine();

			ImGuiTreeNodeFlags flags = isDesaturate ? ImGuiTreeNodeFlags_DefaultOpen : ImGuiTreeNodeFlags_Leaf;

			if (ImGui::TreeNodeEx("Desaturate", flags | ImGuiTreeNodeFlags_Framed))
			{
				if (isDesaturate)
				{
					/* Desaturate Power */
					{
						float desaturatePower = mParticleSystem->GetDesaturatePower();
						if (ImGui::DragFloat("Desaturate Power", &desaturatePower, 0.01f, 0, FLT_MAX))
						{
							mParticleSystem->SetDesaturatePower(desaturatePower);
						}
					}
				}
				ImGui::TreePop();
			}
		}

		auto colorTex = mParticleSystem->GetColorTexture();
		if (colorTex)
		{
			if (ImGui::TreeNodeEx("Color Texture", TREE_NODE_FLAGS))
			{
				colorTex->RenderImgui(256, 256);
				ImGui::TreePop();
			}
		}

		auto alphaTex = mParticleSystem->GetAlphaTexture();
		if (alphaTex)
		{
			if (ImGui::TreeNodeEx("Alpha Texture", TREE_NODE_FLAGS))
			{
				alphaTex->RenderImgui(256, 256);
				ImGui::TreePop();
			}
		}

		ImGui::TreePop();
	}
}