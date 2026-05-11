#include "core/entities/boatentity.h"

#include "imgui/imgui.h"
#include "core/imguiflags.h"

#include <Windows.h>

BoatEntity::BoatEntity()
	: Entity("Boat")
{
}

BoatEntity::~BoatEntity()
{
}

void BoatEntity::UpdateSelf(double delta)
{
	/* Hande Input */
	{
		if (GetAsyncKeyState('W') & 0x8000)
		{
			mInputTimer += delta;
			if (mInputTimer > mTimeToMaxInput)
				mInputTimer = mTimeToMaxInput;
		}
		else
		{
			mInputTimer -= delta;
			if (mInputTimer < 0.0f)
				mInputTimer = 0.0f;
		}

		mUserInput = mInputTimer / mTimeToMaxInput;
	}

	mThrustForce = (mUserInput * mTotalEfficiency * mEnginePower) / (mHullEfficiency * mVelocity * (1 - mWakeFactor));

	mWaterViscosity = CalculateWaterViscosity(WATER_TEMPERATURE, SALT_WATER_CONSTANT);

	float visc = CalculateWaterViscosity(WATER_TEMPERATURE, SALT_WATER_CONSTANT);
	float rey2 = CalculateReynolds(1000, 8, 20, 0.0013);

	mReynoldsNumber = CalculateReynolds(
		WATER_DENSITY, 
		BOAT_LENGTH, 
		mVelocity, 
		mWaterViscosity
	);

	float cf = CalculateCf(mReynoldsNumber);
	float cr = 0.0f; // We don't care about dynamic waves
	mCh = cf + cr;

	constexpr float AREA_UNDER_WATER_RATIO = 0.5f;
	mWaterDragForce = CalculateWaterDragForce(
		WATER_DENSITY,
		(3.0f * 1.0f) * AREA_UNDER_WATER_RATIO, // Cube is 1x1m and is positioned at 0 height meaning half is under the water
		mCh,
		mVelocity
    );

	float totalForce = mThrustForce - mWaterDragForce;

	mAcceleration = totalForce / mMass;

	mVelocity += mAcceleration * delta;

	DirectX::XMFLOAT3 velocityVector = transform.GetForwardDir3f();
	velocityVector.x *= mVelocity;
	velocityVector.y *= mVelocity;
	velocityVector.z *= mVelocity;

	transform.MoveX(velocityVector.x * static_cast<float>(delta));
	transform.MoveY(velocityVector.y * static_cast<float>(delta));
	transform.MoveZ(velocityVector.z * static_cast<float>(delta));
}

void BoatEntity::RenderSelf(RenderServer& renderServer)
{

}

static void DragPercentage(const std::string& name, float& val) 
{
	float percentage = val * 100.0f;
	if (ImGui::DragFloat(name.c_str(), &percentage, 0.01f, 0, 100, "%.2f %%")) 
    {
		val = percentage / 100.0f;
    }
}

void BoatEntity::RenderImguiSelf()
{
	ImGui::TextColored(ImVec4(0, 1, 0, 1), "Reynolds: %.2f", mReynoldsNumber);
	ImGui::TextColored(ImVec4(0, 1, 0, 1), "Water Viscosity %f pas", mWaterViscosity);
	ImGui::TextColored(ImVec4(0, 1, 0, 1), "CH %f", mCh);

	ImGui::TextColored(ImVec4(0, 1, 0, 1), "User Input %.2f", mUserInput);
	ImGui::Text("Thrust Force %.2f", mThrustForce);

	if (ImGui::TreeNodeEx("Constants", TREE_NODE_FLAGS)) 
	{
		ImGui::DragFloat("Wake Factor", &mWakeFactor, 0.001f, 0, FLT_MAX);
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
		ImGui::TreePop();
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