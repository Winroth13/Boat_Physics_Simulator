#pragma once
#include "core/entities/entity.h"
#include "core/entities/modelentity.h"

#include <DirectXMath.h>

class BoatEntity : public Entity
{
public:
	BoatEntity();
	~BoatEntity();

	void SetMotorHingeEntity(Entity* e) { mMotorHingeEntity = e; }

protected:
	virtual void BeginSelf(RenderServer& renderServer) override;
	virtual void UpdateSelf(double delta) override;
	virtual void RenderSelf(RenderServer& renderServer) override;

	void Input(float delta);

	float CalculateWaterViscosity(float waterTemperature, float waterViscosityFactor);

	float CalculateReynolds(
		float waterDensity,
		float boatLength,
		float boatVelocity,
		float viscosity
	);

	/* Empirical calculation of Cf for boat typical shape */
	float CalculateCf(float reynoldsNumber);

	float CalculateWaterDragForce(
		float waterDensity,
		float areaUnderWater,
		float CH,
		float boatVelocity
	);

	float CalculateMomentOfInertia(float length, float width, float mass);

	virtual void RenderImguiSelf() override;

private:
	static constexpr float SALT_WATER_CONSTANT = 1.08f;
	static constexpr float FRESH_WATER_CONSTANT = 1;
	static constexpr float WATER_DENSITY = 1000;
	static constexpr float WATER_TEMPERATURE = 273.15f + 10;

	static constexpr float BOAT_WIDTH = 1.3f;
	static constexpr float BOAT_LENGTH = 3.3f;
	static constexpr float BOAT_HEIGHT = 0.5f;
	static constexpr float BOAT_LENGTH_TO_MOTOR = BOAT_LENGTH / 2;

	static constexpr float BOAT_MAX_TURN_ANGLE = DirectX::XMConvertToRadians(30);

	static constexpr float UPDATE_RATE = 1 / 60.0f;

	Entity* mMotorHingeEntity = nullptr;

	float mUpdateTimer = 0.0f;

	// Xi
	float mForwardUserInput = 0.0f;
	float mForwardTimeToMaxInput = 1.0f;
	float mForwardInputTimer = 0.0f;

	float mTurnUserInput = 0.0f;
	float mTurnTimeToMaxInput = 2.0f;
	float mTurnInputTimer = mTurnTimeToMaxInput / 2.0f;

	float GetTurnAngle() { return mTurnUserInput * BOAT_MAX_TURN_ANGLE; }

	/* Constants */
	float mMass = 100.0f;
	float mWakeFactor = 0.06f;
	// Eta_p
	float mTotalEfficiency = 0.7f; // How much of the engine power that is conserved
	// Eta_H
	float mHullEfficiency = 0.95f;

	float mEnginePower = 20000;

	float mVelocity = 20;
	float mAcceleration = 0;

	float mAngularVelocity = 0.0f;

	/* Forces */
	float mThrustForce = 0.0f;
	float mWaterDragForce = 0.0f;

	float mWaterViscosity = 0.0f;
	float mReynoldsNumber = 0.0f;
	float mCh = 0.0f;
};