#include "core/entities/boatentity.h"

#include "graphics/models/objmodel.h"
#include "graphics/meshes/quadmesh.h"

#include "graphics/shaders/vertexshader.h"
#include "graphics/materials/material.h"

#include "imgui/imgui.h"
#include "core/imguiflags.h"

#include "core/entities/cameraentity.h"
#include "graphics/meshes/quadmesh.h"
#include "graphics/textures/imagetexture2d.h"
#include "core/scene.h"

#include <Windows.h>

BoatEntity::BoatEntity()
	: Entity("Boat")
{
}

BoatEntity::~BoatEntity()
{
}

void BoatEntity::BeginSelf(RenderServer& renderServer)
{
}

void BoatEntity::UpdateSelf(double delta)
{
	mUpdateTimer += delta;

	if (mUpdateTimer < UPDATE_RATE)
		return;

	mUpdateTimer -= UPDATE_RATE;
	delta = static_cast<double>(UPDATE_RATE);

	Input(static_cast<float>(delta));

	mThrustForce = (mForwardUserInput * mTotalEfficiency * mEnginePower) /
		(mHullEfficiency * mVelocity * (1 - mWakeFactor));

	float turnAngle = GetTurnAngle();

	mMotorHingeEntity->transform.SetYaw(-turnAngle);

	float inertia = mThrustForce * sinf(turnAngle) * BOAT_LENGTH_TO_MOTOR;
	float momentOfInertia = CalculateMomentOfInertia(BOAT_LENGTH, BOAT_WIDTH, mMass);

	float angularAcceleration = inertia / momentOfInertia;
	mAngularVelocity = angularAcceleration * delta;

	constexpr float A = 2000.0f;
	constexpr float k = 10.0f;
	float waterInertia = A * (expf(k * mAngularVelocity) - 1);
	float angularDeceleration = waterInertia / momentOfInertia;

	mAngularVelocity -= angularDeceleration * delta;

	transform.RotateY(mAngularVelocity);

	mWaterViscosity = CalculateWaterViscosity(WATER_TEMPERATURE, SALT_WATER_CONSTANT);

	mReynoldsNumber = CalculateReynolds(
		WATER_DENSITY,
		BOAT_LENGTH,
		mVelocity,
		mWaterViscosity
	);

	float cf = CalculateCf(mReynoldsNumber);
	float cr = 0.0f; // We don't care about dynamic waves
	mCh = cf + cr;

	constexpr float AREA_UNDER_WATER_RATIO = 0.5f; // TODO: Do not hardcode this
	mWaterDragForce = CalculateWaterDragForce(
		WATER_DENSITY,
		(BOAT_WIDTH * BOAT_HEIGHT) * AREA_UNDER_WATER_RATIO,
		mCh,
		mVelocity
	);

	mAcceleration = (mThrustForce - mWaterDragForce) / mMass;

	mVelocity += mAcceleration * static_cast<float>(delta);

	transform.MoveForward(mVelocity * static_cast<float>(delta));
}

void BoatEntity::RenderSelf(RenderServer& renderServer)
{
}

static inline bool IsKeyDown(int vKey)
{
	return GetAsyncKeyState(vKey) & 0x8000;
}

void BoatEntity::Input(float delta) {
	if (IsKeyDown('W'))
	{
		mForwardInputTimer += delta;
		if (mForwardInputTimer > mForwardTimeToMaxInput)
			mForwardInputTimer = mForwardTimeToMaxInput;
	}
	else if (IsKeyDown('S'))
	{
		mForwardInputTimer -= delta;
		if (mForwardInputTimer < 0.0f)
			mForwardInputTimer = 0.0f;
	}

	mForwardUserInput = mForwardInputTimer / mForwardTimeToMaxInput;

	if (IsKeyDown('D'))
	{
		mTurnInputTimer += delta;
		if (mTurnInputTimer > mTurnTimeToMaxInput)
			mTurnInputTimer = mTurnTimeToMaxInput;
	}
	else if (IsKeyDown('A'))
	{
		mTurnInputTimer -= delta;
		if (mTurnInputTimer < 0.0f)
			mTurnInputTimer = 0.0f;
	}

	mTurnUserInput = (mTurnInputTimer / mTurnTimeToMaxInput) * 2.0f - 1.0f;
}

static void DragPercentage(const std::string& name, float& val)
{
	float percentage = val * 100.0f;
	if (ImGui::DragFloat(name.c_str(), &percentage, 0.01f, 0, 100, "%.2f %%"))
	{
		val = percentage / 100.0f;
	}
}

float BoatEntity::CalculateWaterViscosity(float waterTemperature, float waterViscosityFactor)
{
	float exponent = 570.6f / (waterTemperature - 140);
	return 2.41e-5f * expf(exponent) * waterViscosityFactor;
}

float BoatEntity::CalculateReynolds(
	float waterDensity,
	float boatLength,
	float boatVelocity,
	float viscosity
)
{
	return (waterDensity * boatLength * boatVelocity) / viscosity;
}

float BoatEntity::CalculateCf(float reynoldsNumber)
{
	return 3.0f / (40.0f * powf(log10f(reynoldsNumber) - 2.0f, 2.0f));
}

float BoatEntity::CalculateWaterDragForce(
	float waterDensity,
	float areaUnderWater,
	float CH,
	float boatVelocity
)
{
	return 0.5f * waterDensity * areaUnderWater * CH * powf(boatVelocity, 2);
}

float BoatEntity::CalculateMomentOfInertia(float length, float width, float mass)
{
	float I = mass / 48.0f;
	I *= (4 * powf(length, 2) + 3 * powf(width, 2));
	return I;
}

void BoatEntity::RenderImguiSelf()
{
	ImGui::TextColored(ImVec4(0, 1, 0, 1), "Reynolds: %.2f", mReynoldsNumber);
	ImGui::TextColored(ImVec4(0, 1, 0, 1), "Water Viscosity %f pas", mWaterViscosity);
	ImGui::TextColored(ImVec4(0, 1, 0, 1), "CH %f", mCh);

	if (ImGui::TreeNodeEx("Input", TREE_NODE_FLAGS))
	{
		ImGui::SliderFloat("Forward Input", &mForwardUserInput, 0, 1);
		ImGui::SliderFloat("Turn Input", &mTurnUserInput, -1, 1);

		ImGui::TreePop();
	}

	if (ImGui::TreeNodeEx("Forces", TREE_NODE_FLAGS))
	{
		ImGui::TextColored(ImVec4(0, 1, 0, 1), "Thrust Force %.2f", mThrustForce);
		ImGui::TextColored(ImVec4(1, 0, 0, 1), "Water Drag Force %.2f", mWaterDragForce);

		ImGui::TextColored(ImVec4(1, 1, 1, 1), "Total Force %.2f", mThrustForce - mWaterDragForce);
		ImGui::TreePop();
	}

	if (ImGui::TreeNodeEx("Constants", TREE_NODE_FLAGS))
	{
		ImGui::DragFloat("Wake Factor", &mWakeFactor, 0.001f, 0, 0.99f);
		ImGui::DragFloat("Mass", &mMass, 1, 0, FLT_MAX, "%.2f kg");
		// TODO: Other constants

		/* Total Efficiency */
		DragPercentage("Total Efficiency", mTotalEfficiency);
		/* Hull Efficiency */
		DragPercentage("Hull Efficiency", mHullEfficiency);

		ImGui::DragFloat("Engine Power", &mEnginePower, 1, 0, FLT_MAX, "%.2f w");

		ImGui::TreePop();
	}

	if (ImGui::TreeNodeEx("Motion", TREE_NODE_FLAGS))
	{
		ImGui::DragFloat("Velocity", &mVelocity, 0.01f, 0, FLT_MAX, "%.1f m/s");
		ImGui::DragFloat("Acceleration", &mAcceleration, 0.01f, 0, FLT_MAX, "%.1f m/s");

		ImGui::DragFloat("Angular Velocity", &mAngularVelocity, 0.01f, 0, FLT_MAX, "%.1f m/s");
		ImGui::TreePop();
	}
}