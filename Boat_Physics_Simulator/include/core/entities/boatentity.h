#pragma once
#include "core/entities/entity.h"
#include "core/entities/modelentity.h"
#include "math/boatmaths.h"

#include "graphics/models/objmodel.h"

#include "boat/areacalculator.h"

#include <memory>
#include <DirectXMath.h>

#define USE_TITLE_SCREEN false

class BoatEntity : public Entity
{
public:
	BoatEntity();
	~BoatEntity();

	void SetMotorHingeEntity(Entity* e) { mMotorHingeEntity = e; }
	void SetPropellerEntity(Entity* e) { mPropellerEntity = e; }
	void SetBoatModelEntity(ModelEntity* e) { mBoatModelEntity = e; }
	void SetGameEntity(Entity* e) { mGameEntity = e; }

protected:
	virtual void BeginSelf(RenderServer& renderServer) override;
	virtual void UpdateSelf(double delta) override;
	virtual void RenderSelf(RenderServer& renderServer) override;

	void Input(float deltaTime);
	virtual void RenderImguiSelf() override;

private:
	enum class BoatState
	{
		DEFAULT,
		ASCENDING
	};

	static std::string BoatStateToString(BoatState state);

	enum class PointCloudType
	{
		BOAT,
		AIR,
		ENGINE,

		COUNT
	};

	PointCloud& GetPointCloud(PointCloudType type) { return mPointClouds[static_cast<int>(type)]; }

	float GetBoatLength() { return GetPointCloud(PointCloudType::BOAT).GetLength(); }
	float GetBoatWidth() { return GetPointCloud(PointCloudType::BOAT).GetWidth(); }
	float GetBoatHeight() { return GetPointCloud(PointCloudType::BOAT).GetHeight(); }

private:
	static constexpr float UPDATE_RATE = 1 / 60.0f;
	static constexpr float GRAVITY = 9.82f;

	static constexpr float SALT_WATER_CONSTANT = 1.08f;
	static constexpr float FRESH_WATER_CONSTANT = 1;
	static constexpr float WATER_DENSITY = 1000;
	static constexpr float WATER_TEMPERATURE = 273.15f + 10;

	static constexpr float BOAT_MAX_TURN_ANGLE = DirectX::XMConvertToRadians(30);

	static constexpr float EQUILIBRIUM_ACCELERATION_THRESHOLD = 0.4f;
	static constexpr float EQUILIBRIUM_VELOCITY_THRESHOLD = 0.3f;

	static constexpr float MIN_VELOCITY = 0.1f;

	DirectX::XMFLOAT3 mCenterOfMass;

	std::unique_ptr<OBJModel> mSphereModel;

	std::vector<PointCloud> mPointClouds;

	AreaCalculator mAreaCalculator;

	Entity* mMotorHingeEntity = nullptr;
	Entity* mPropellerEntity = nullptr;
	ModelEntity* mBoatModelEntity = nullptr;
	Entity* mRootEntity = nullptr;
	Entity* mGameEntity = nullptr;

	float mUpdateTimer = 0.0f;

	BoatState mState = BoatState::DEFAULT;

	/* Dampening */
	static constexpr float GAMMA = 5;
	float mDistanceToSurface = 0.0f;
	float mTimeAscending = 0.0f;

	// Xi
	float mForwardUserInput = 0.0f;
	float mForwardTimeToMaxInput = 1.0f;
	float mForwardInputTimer = 0.0f;

	float mTurnUserInput = 0.0f;
	float mTurnTimeToMaxInput = 2.0f;
	float mTurnInputTimer = mTurnTimeToMaxInput / 2.0f;

	float GetTurnAngle() { return mTurnUserInput * BOAT_MAX_TURN_ANGLE; }

	/* Constants */
	float mMass = 1000.0f;
	float mWakeFactor = 0.06f;
	// Eta_p
	float mTotalEfficiency = 0.7f; // How much of the engine power that is conserved
	// Eta_H
	float mHullEfficiency = 0.95f;

	float mEnginePower = 20000;

	DirectX::XMFLOAT3 mVelocity = { 0, 0, 20 };
	float forwardAcceleration = 0.0f;

	DirectX::XMFLOAT3 mAcceleration = { 0, 0, 0 };

	float mAngularVelocity = 0.0f;

	/* Forces */
	float mThrustForce = 0.0f;
	DirectX::XMFLOAT3 mWaterDragForce = { 0.0f, 0.0f, 0.0f };

	float mWaterViscosity = 0.0f;
	float mReynoldsNumber = 0.0f;
	float mCh = 0.0f;

	bool mPause = USE_TITLE_SCREEN; // <-- SET TRUE FOR TITLE SCREEN :)

	float mVolumeUnderWater = 0.0f;

	float mFrontAreaUnderWater = 0.0f;
	float mBottomAreaUnderWater = 0.0f;
};