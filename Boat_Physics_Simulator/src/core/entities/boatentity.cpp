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
#include "math/pointcloud.h"

#include "math/mathfunctions.h"

#include <string>
#include <Windows.h>

using namespace DirectX;

BoatEntity::BoatEntity()
	: Entity("Boat")
{
}

BoatEntity::~BoatEntity()
{
}

void BoatEntity::BeginSelf(RenderServer& renderServer)
{
	if (!mAreaCalculator.Create())
	{
		Logger::Error("Failed to create area calculator");
		throw std::runtime_error("");
	}

	auto vShader = std::make_shared<VertexShader>("resources/VertexShader.cso");
	mSphereModel = std::make_unique<OBJModel>("assets/pointclouds/point_sphere.obj", vShader);

	PointCloud boatCloud("assets/pointclouds/new/point_cloud_boat.obj", 1000);
	PointCloud airCloud("assets/pointclouds/new/point_cloud_air.obj", 0.125f, 1.225f);
	PointCloud engineCloud("assets/pointclouds/new/point_cloud_engine.obj", 300);

	boatCloud.SetPointRadius(0.025f);
	engineCloud.SetPointRadius(0.025f);

	mPointClouds.resize((int)PointCloudType::COUNT);

	mPointClouds[static_cast<int>(PointCloudType::BOAT)] = boatCloud;
	mPointClouds[static_cast<int>(PointCloudType::AIR)] = airCloud;
	mPointClouds[static_cast<int>(PointCloudType::ENGINE)] = engineCloud;

	mCenterOfMass = CalculateCenterOfMass(mPointClouds);
	mMass = boatCloud.GetTotalMass() + engineCloud.GetTotalMass();

	mMomentOfIntertiaMatrix = CalculateMomentOfInertiaMatrix(mPointClouds, mCenterOfMass);
}

void BoatEntity::UpdateSelf(double deltaTime)
{
	if (mPause)
		return;

	if (!mUncapSimulationSpeed) {
		mUpdateTimer += static_cast<float>(deltaTime);
		if (mUpdateTimer < UPDATE_RATE)
			return;

		mUpdateTimer -= UPDATE_RATE;
	}

	float delta = UPDATE_RATE;

	Input(delta);

	/* Update Camera FOV */
	XMVECTOR velocity = XMLoadFloat3(&mVelocity);
	float t = (XMVectorGetX(XMVector3Length(velocity)) - MIN_VELOCITY) / (MAX_VELOCITY - MIN_VELOCITY);
	t = t * t;
	float fov = MIN_FOV + t * (MAX_FOV - MIN_FOV);

	if (fov > MAX_FOV)
		fov = MAX_FOV;
	else if (fov < MIN_FOV)
		fov = MIN_FOV;

	mCameraEntity->SetFov(fov);

	XMVECTOR acceleration = XMLoadFloat3(&mAcceleration);

	/* Calculate Area */
	mFrontAreaUnderWater = mAreaCalculator.CalculateArea(
		mBoatModelEntity->GetModel(),
		transform,
		{ 0, DirectX::XM_PI, 0 }
	);

	mBottomAreaUnderWater = mAreaCalculator.CalculateArea(
		mBoatModelEntity->GetModel(),
		transform,
		{ -DirectX::XM_PIDIV2, 0, 0 }
	);

	XMVECTOR forward = transform.GetForwardDir();
	float velocityForwardScalar = XMVectorGetX(XMVector3Dot(velocity, forward));

	/* Calculate Forward Thrust Force */
	{
		if (mPropellerEntity->GetGlobalPosition().y <= 0)
		{
			if (mForwardUserInput > 0.0f && velocityForwardScalar < MIN_VELOCITY) // If you start the boat when standing still
			{
				velocityForwardScalar = MIN_VELOCITY;
			}

			if (velocityForwardScalar >= MIN_VELOCITY) // If the velocity is low enough, the boat stops
			{
				float numerator = (mForwardUserInput * mTotalEfficiency * mEnginePower);
				float denominator = (mHullEfficiency * velocityForwardScalar * (1 - mWakeFactor));
				mThrustForce = numerator / denominator;
			}
			else
			{
				velocity *= XMVectorSet(0.0f, 1.0f, 0.0f, 1.0f);
				mThrustForce = 0;
			}
		}
		else
		{
			mThrustForce = 0;
		}
	}

	XMMATRIX transformMatrix = mBoatModelEntity->GetGlobalTransform();

	std::vector<PointCloud> aboveWaterPointClouds;
	std::vector<PointCloud> belowWaterPointClouds;

	/* Calculate Boat Volumes */
	{
		PointCloud above;
		PointCloud below;

		SplitPointCloud(GetPointCloud(PointCloudType::BOAT), transformMatrix, 0.0f, above, below);
		mVolumeUnderWater = below.GetVolume();

		aboveWaterPointClouds.push_back(above);
		belowWaterPointClouds.push_back(below);
	}

	/* Calculate Air Volumes */
	{
		PointCloud above;
		PointCloud below;

		SplitPointCloud(GetPointCloud(PointCloudType::AIR), transformMatrix, 0.0f, above, below);
		mVolumeUnderWater += below.GetVolume();

		aboveWaterPointClouds.push_back(above);
		belowWaterPointClouds.push_back(below);
	}

	/* Calculate Engine Volumes */
	{
		PointCloud above;
		PointCloud below;

		SplitPointCloud(GetPointCloud(PointCloudType::ENGINE), transformMatrix, 0.0f, above, below);
		mVolumeUnderWater += below.GetVolume();

		aboveWaterPointClouds.push_back(above);
		belowWaterPointClouds.push_back(below);
	}

	/* Calculate Angular Velocity Yaw */
	{
		float turnAngle = GetTurnAngle();

		mMotorHingeEntity->transform.SetYaw(-turnAngle);

		XMFLOAT3 propellerGlobalPosition = mPropellerEntity->GetGlobalPosition();
		propellerGlobalPosition.y = 0.0f;

		XMVECTOR centerOfMass = XMLoadFloat3(&mCenterOfMass);
		centerOfMass = XMVectorSetY(centerOfMass, 0);

		XMVECTOR propellerPosition = XMLoadFloat3(&propellerGlobalPosition);
		XMVECTOR center = XMVector3Transform(centerOfMass, mRootEntity->GetGlobalTransform());
		float distanceToCentre = XMVectorGetX(XMVector3Length(propellerPosition - center));

		mEngineTurnTorque = mThrustForce * sinf(turnAngle) * distanceToCentre;
		float momentOfInertia = CalculateMomentOfInertia(GetBoatLength(), GetBoatWidth(), mMass);

		mAngularAcceleration.y = mEngineTurnTorque / momentOfInertia;
		mAngularVelocity.y = mAngularAcceleration.y * delta;

		float totalVolume = 0.0f;
		totalVolume += GetPointCloud(PointCloudType::BOAT).GetVolume();
		totalVolume += GetPointCloud(PointCloudType::AIR).GetVolume();
		totalVolume += GetPointCloud(PointCloudType::ENGINE).GetVolume();

		float volumeUnderWaterRatio = mVolumeUnderWater / totalVolume;

		constexpr float A = 500.0f;
		constexpr float k = 6.0f;
		mWaterTurnTorque = -A * (expf(k * mAngularVelocity.y) - 1);
		float angularDeceleration = (mWaterTurnTorque / momentOfInertia) * (2 * volumeUnderWaterRatio); // Because the equation assumes half the volume is under water

		mAngularVelocity.y += angularDeceleration * delta;

		transform.RotateY(mAngularVelocity.y);
		XMMATRIX rotationMatrix = XMMatrixRotationY(mAngularVelocity.y);
		velocity = XMVector3Transform(velocity, rotationMatrix);
	}

	mWaterViscosity = CalculateWaterViscosity(WATER_TEMPERATURE, SALT_WATER_CONSTANT);

	/* Calculate forward water drag force */
	{
		XMVECTOR forward = transform.GetForwardDir();
		float velocityTotalScalar = XMVectorGetX(XMVector3Length(velocity));
		float velocityForwardScalar = XMVectorGetX(XMVector3Dot(velocity, forward));

		mReynoldsNumber = CalculateReynolds(
			WATER_DENSITY,
			GetBoatLength(),
			velocityTotalScalar,
			mWaterViscosity
		);

		float cf = CalculateCf(mReynoldsNumber);
		float cr = 0.0f; // We don't care about dynamic waves
		mCh = cf + cr;

		mWaterDragForce.z = CalculateWaterDragForce(
			WATER_DENSITY,
			mFrontAreaUnderWater,
			mCh,
			velocityForwardScalar
		);
		XMVECTOR waterDragForceV = XMLoadFloat3(&mWaterDragForce);
		XMMATRIX rotationMatrix = XMMatrixRotationY(mAngularVelocity.y);
		waterDragForceV = XMVector3Transform(waterDragForceV, rotationMatrix);
		XMStoreFloat3(&mWaterDragForce, waterDragForceV);
	}

	float accelerationForwardScalar = (mThrustForce - mWaterDragForce.z) / mMass;
	acceleration = accelerationForwardScalar * transform.GetForwardDir(); // Forward Thrust Force

	/* Calculate and apply torque */
	{
		XMVECTOR centerOfMass = XMLoadFloat3(&mCenterOfMass);
		XMVECTOR center = XMVector3Transform(centerOfMass, mRootEntity->GetGlobalTransform());
		XMVECTOR right = transform.GetRightDir();

		/* Engine Torque */
		{
			XMFLOAT3 propellerGlobalPosition = mPropellerEntity->GetGlobalPosition();
			XMVECTOR propellerPosition = XMLoadFloat3(&propellerGlobalPosition);
			float propellerToCenter = XMVectorGetX(XMVector3Length(propellerPosition - center));

			XMVECTOR torqueUnitVector = -XMVector3Normalize(
				XMVector3Cross(propellerPosition - center, right)
			);

			XMVECTOR forwardForceVector = mRootEntity->transform.GetForwardDir() * mThrustForce;
			float torqueForce = XMVectorGetX(XMVector3Dot(forwardForceVector, torqueUnitVector));

			mEnginePitchTorque = propellerToCenter * torqueForce;
		}

		/* Gravitational torque */

		// Mass over water falling
		{
			// Only count boat and engine clouds, since air does not contribute
			std::vector<PointCloud> aboveWaterBoatAndEngine;
			aboveWaterBoatAndEngine.push_back(aboveWaterPointClouds[static_cast<int>(PointCloudType::BOAT)]);
			aboveWaterBoatAndEngine.push_back(aboveWaterPointClouds[static_cast<int>(PointCloudType::ENGINE)]);

			XMFLOAT3 massCenterAbovef = CalculateCenterOfMass(aboveWaterBoatAndEngine);
			XMVECTOR massCenterAbove = XMLoadFloat3(&massCenterAbovef);
			massCenterAbove = XMVector3Transform(massCenterAbove, mRootEntity->GetGlobalTransform());

			float massAboveToCenter = XMVectorGetX(XMVector3Length(massCenterAbove - center));

			XMVECTOR torqueUnitVector = -XMVector3Normalize(
				XMVector3Cross(massCenterAbove - center, right)
			);

			float fallingMass = aboveWaterBoatAndEngine[0].GetTotalMass() + aboveWaterBoatAndEngine[1].GetTotalMass();
			XMVECTOR gravitationalForce = XMVectorSet(0, -GRAVITY, 0, 0) * fallingMass;
			float torqueForce = XMVectorGetX(XMVector3Dot(gravitationalForce, torqueUnitVector));

			mGravitationalPitchTorque = massAboveToCenter * torqueForce;
		}

		// Mass under water ascending
		{
			XMFLOAT3 volumeCenterBelowf = CalculateCenterOfVolume(belowWaterPointClouds);
			XMVECTOR volumeCenterBelow = XMLoadFloat3(&volumeCenterBelowf);
			volumeCenterBelow = XMVector3Transform(volumeCenterBelow, mRootEntity->GetGlobalTransform());

			float volumeBelowToCenter = XMVectorGetX(XMVector3Length(volumeCenterBelow - center));

			XMVECTOR torqueUnitVector = -XMVector3Normalize(
				XMVector3Cross(volumeCenterBelow - center, right)
			);

			XMVECTOR waterForce = XMVectorSet(0, 1, 0, 0) * mVolumeUnderWater * WATER_DENSITY * GRAVITY;
			float torqueForce = XMVectorGetX(XMVector3Dot(waterForce, torqueUnitVector));

			mBuoyancyPitchTorque = volumeBelowToCenter * torqueForce;
		}
		/* Resulting torque */
		XMVECTOR localRight = XMVectorSet(1, 0, 0, 0);
		float momentOfIntertiaLength = XMVectorGetX(XMVector3Length(
			XMVector3Transform(localRight, mMomentOfIntertiaMatrix))
		);
		XMVECTOR momentOfIntertiaV = localRight * momentOfIntertiaLength;
		float momentOfInertia = XMVectorGetX(XMVector3Length(momentOfIntertiaV));

		mAngularAcceleration.x = (mEnginePitchTorque + mGravitationalPitchTorque + mBuoyancyPitchTorque) / momentOfInertia;
		mAngularVelocity.x += mAngularAcceleration.x * delta;

		/* Water torque dampening */
		float totalVolume = 0.0f;
		totalVolume += GetPointCloud(PointCloudType::BOAT).GetVolume();
		totalVolume += GetPointCloud(PointCloudType::AIR).GetVolume();
		totalVolume += GetPointCloud(PointCloudType::ENGINE).GetVolume();

		float volumeUnderWaterRatio = mVolumeUnderWater / totalVolume;

		constexpr float A = 1000.0f;
		constexpr float k = 6.0f;
		mWaterPitchTorque = -A * (expf(k * mAngularVelocity.x) - 1);
		float angularDeceleration = mWaterPitchTorque / momentOfInertia * (2 * volumeUnderWaterRatio); // Because the equation assumes half the volume is under water

		mAngularVelocity.x += angularDeceleration * delta;
	}

	switch (mState)
	{
	case BoatState::DEFAULT:
	{
		/* Calculate upwards water drag force */
		{
			float velocityUpScalar = fabsf(XMVectorGetY(velocity));

			mWaterDragForce.y = CalculateWaterDragForce(
				WATER_DENSITY,
				mBottomAreaUnderWater,
				mCh,
				velocityUpScalar
			);
		}
		/* The hull works like a wing on the water */
		float wingForce = 0.5f * WATER_DENSITY * mFrontAreaUnderWater * mCh * velocityForwardScalar * velocityForwardScalar;
		float wingAcceleration = wingForce / mMass;
		mWingForce = wingForce;

		/* Water displacement crates an upwards force */
		float liftAcceleration = WATER_DENSITY * mVolumeUnderWater / mMass * GRAVITY;

		acceleration += XMVectorSet(0, wingAcceleration + liftAcceleration + mWaterDragForce.y / mMass - GRAVITY, 0, 0);

		velocity += acceleration * delta;

		/*
		*	When Lift Acceleration is almost equal to Gravity,
		*	force acceleration and velocity to 0 to prevent
		*	too much bouncing on the water.
		*
		*   It's probably the balls' fault that we have to do this
		*	since it's impossible to find the perfect equilibrium.
		*/
		if (
			fabsf(wingAcceleration + liftAcceleration - GRAVITY) < EQUILIBRIUM_ACCELERATION_THRESHOLD &&
			fabsf(XMVectorGetY(velocity)) < EQUILIBRIUM_VELOCITY_THRESHOLD
			)
		{
			acceleration *= XMVectorSet(1, 0, 1, 1);
			velocity *= XMVectorSet(1, 0, 1, 1);
		}

		transform.MoveY(XMVectorGetY(velocity) * delta);

		/* Check if we should start ascending */
		if (
			mVelocity.y < 0.0f &&
			XMVectorGetY(velocity) >= 0.0f &&
			transform.GetPosition3f().y < -ASCENDING_END_THRESHOLD
			)
		{
			mState = BoatState::ASCENDING;
			mDistanceToSurface = transform.GetPosition3f().y;
			mTimeAscending = 0.0f;
		}
	}
	break;

	case BoatState::ASCENDING:
	{
		mTimeAscending += delta;

		acceleration *= XMVectorSet(1, 0, 1, 1);
		velocity += acceleration * delta;

		XMFLOAT3 position = transform.GetPosition3f();
		position.y = CalculateDampenedBoatY(mDistanceToSurface, mTimeAscending, GAMMA);

		/* Check if we have finished ascending */
		if (position.y >= -ASCENDING_END_THRESHOLD)
		{
			mState = BoatState::DEFAULT;
			position.y = 0.0f;
		}

		transform.SetPosition(position);
	}
	break;
	}

	DirectX::XMStoreFloat3(&mVelocity, velocity);
	DirectX::XMStoreFloat3(&mAcceleration, acceleration);

	/* Always rotate pitch */
	mRootEntity->transform.RotateX(mAngularVelocity.x * delta);

	/* Always move in X and Z */
	transform.MoveX(mVelocity.x * delta);
	transform.MoveZ(mVelocity.z * delta);
}

void BoatEntity::RenderSelf(RenderServer& renderServer)
{
	// This can be used to render the point clouds, if you want

	// constexpr float radius = 0.025f;

	// Transform pTransform;
	// pTransform.SetPosition(mCenterOfMass);
	// pTransform.SetScale(radius, radius, radius);

	// renderServer.PushMesh(mSphereModel->GetMesh(0), pTransform.GetMatrix() * transform.GetMatrix());
	// renderServer.PushMaterial(mSphereModel->GetMaterial(0));

	// for (auto& point : GetPointCloud(PointCloudType::AIR).GetPoints())
	// {
	// 	Transform pTransform;
	// 	pTransform.SetPosition(point.position);
	// 	pTransform.SetScale(radius, radius, radius);

	// 	renderServer.PushMesh(mSphereModel->GetMesh(0), pTransform.GetMatrix() * mBoatModelEntity->GetGlobalTransform());
	// 	renderServer.PushMaterial(mSphereModel->GetMaterial(0));
	// }
}

std::string BoatEntity::BoatStateToString(BoatState state)
{
	switch (state)
	{
	case BoatState::DEFAULT:
		return "Default";

	case BoatState::ASCENDING:
		return "Ascending";

	default:
		return "How did this happen?";
	}
}

static inline bool IsKeyDown(int vKey)
{
	return GetAsyncKeyState(vKey) & 0x8000;
}

void BoatEntity::Input(float delta)
{
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

void BoatEntity::RenderImguiSelf()
{
	if (ImGui::TreeNodeEx("Settings", TREE_NODE_FLAGS))
	{
		ImGui::SeparatorText("Simulation");
		ImGui::Checkbox("Pause", &mPause);
		ImGui::Checkbox("Uncap Simulation", &mUncapSimulationSpeed);
		ImGui::SameLine();
		ImGui::TextColored(ImVec4(1, 0.5f, 0.5f, 1.0f), "(Expensive)");

		ImGui::SeparatorText("Boat");
		ImGui::DragFloat("Wake Factor", &mWakeFactor, 0.001f, 0, 0.99f);
		DragPercentage("Total Efficiency", mTotalEfficiency);
		DragPercentage("Hull Efficiency", mHullEfficiency);
		ImGui::DragFloat("Engine Power", &mEnginePower, 50, 0, FLT_MAX, "%.2f w");

		ImGui::TreePop();
	}

	if (ImGui::TreeNodeEx("General", TREE_NODE_FLAGS))
	{
		ImGui::Text("State: %s", BoatStateToString(mState).c_str());

		ImGui::SeparatorText("Boat Info");
		ImGui::Text("Width: %.2f m", GetBoatWidth());
		ImGui::Text("Height: %.2f m", GetBoatHeight());
		ImGui::Text("Length: %.2f m", GetBoatLength());
		ImGui::Text("Mass: %.2f kg", mMass);

		float totalVolume = 0;
		totalVolume += GetPointCloud(PointCloudType::BOAT).GetVolume();
		totalVolume += GetPointCloud(PointCloudType::AIR).GetVolume();
		totalVolume += GetPointCloud(PointCloudType::ENGINE).GetVolume();

		ImGui::SeparatorText("Volume");
		ImGui::Text("Volume: %.2f m^3", totalVolume);

		float percentBelowWater = (mVolumeUnderWater / totalVolume) * 100;

		ImGui::Text("Volume Below Water: %.1f %%", percentBelowWater);
		ImGui::Text("Volume Above Water: %.1f %%", 100 - percentBelowWater);

		ImGui::SeparatorText("Area");
		ImGui::Text("Front Area Below Water: %.3f m^2", mFrontAreaUnderWater);
		ImGui::Text("Bottom Area Below Water: %.3f m^2", mBottomAreaUnderWater);

		ImGui::SeparatorText("Boat Typical Shape");
		ImGui::TextColored(ImVec4(1, 1, 1, 1), "Reynolds: %.2f", mReynoldsNumber);
		ImGui::TextColored(ImVec4(1, 1, 1, 1), "Water Viscosity: %f pas", mWaterViscosity);
		ImGui::TextColored(ImVec4(1, 1, 1, 1), "CH: %f", mCh);

		ImGui::SeparatorText("Speed");

		XMVECTOR velocityV = XMLoadFloat3(&mVelocity);
		float velocityScalar = XMVectorGetX(XMVector3Length(velocityV));
		ImGui::Text("%.2f kn", velocityScalar * 1.94384f);
		ImGui::Text("%.2f km/h", velocityScalar * 3.6f);
		ImGui::Text("%.2f m/s", velocityScalar);

		ImGui::TreePop();
	}

	if (ImGui::TreeNodeEx("Forces", TREE_NODE_FLAGS))
	{
		ImGui::SeparatorText("Translation");

		ImGui::Text("Thrust Force: ");
		ImGui::SameLine();
		ImGui::TextColored(ImVec4(0.5f, 0.5f, 1.0f, 1.0f), "Z: ");
		ImGui::SameLine();
		ImGui::Text("%.2f N", mThrustForce);

		ImGui::Text("Wing Force: ");
		ImGui::SameLine();
		ImGui::TextColored(ImVec4(0.5f, 1.0f, 0.5f, 1.0f), "Y: ");
		ImGui::SameLine();
		ImGui::Text("%.2f N", mWingForce);

		ImGui::Text("Water Drag Force: ");
		ImGui::SameLine();
		ImGui::TextColored(ImVec4(0.5f, 1.0f, 0.5f, 1), "Y: ");
		ImGui::SameLine();
		ImGui::Text("%.2f ", mWaterDragForce.y);
		ImGui::SameLine();
		ImGui::TextColored(ImVec4(0.5f, 0.5f, 1.0f, 1.0f), "Z: ");
		ImGui::SameLine();
		ImGui::Text("%.2f ", mWaterDragForce.z);
		ImGui::SameLine();
		ImGui::Text(" N");

		ImGui::SeparatorText("Rotation");

		ImGui::Text("Engine Turn Torque: %.2f Nm", mEngineTurnTorque);
		ImGui::Text("Water Turn Torque: %.2f Nm", mWaterTurnTorque);

		ImGui::SeparatorText("Pitch");

		ImGui::Text("Engine Pitch Torque: %.2f Nm", mEnginePitchTorque);
		ImGui::Text("Gravitational Pitch Torque: %.2f Nm", mGravitationalPitchTorque);
		ImGui::Text("Bouyancy Pitch Torque: %.2f Nm", mBuoyancyPitchTorque);
		ImGui::Text("Water Pitch Torque: %.2f Nm", mWaterPitchTorque);

		ImGui::TreePop();
	}

	if (ImGui::TreeNodeEx("Motion", TREE_NODE_FLAGS))
	{
		XMFLOAT3 rootAngles = mRootEntity->transform.GetAngles3f();
		float rootPitchDeg = DirectX::XMConvertToDegrees(rootAngles.x);

		if (ImGui::DragFloat("Pitch Angle", &rootPitchDeg, 1.0f, -FLT_MAX, FLT_MAX, "%.1f deg"))
		{
			rootAngles.x = XMConvertToRadians(rootPitchDeg);
			mRootEntity->transform.SetAngles(rootAngles);
		}

		ImGui::DragFloat3("Velocity", &mVelocity.x, 0.01f, 0, FLT_MAX, "%.2f m/s");
		ImGui::DragFloat3("Acceleration", &mAcceleration.x, 0.01f, 0, FLT_MAX, "%.2f m/s^2");

		ImGui::DragFloat3("Angular Velocity", &mAngularVelocity.x, 0.01f, 0, FLT_MAX, "%.2f rad/s");
		ImGui::DragFloat3("Angular Acceleration", &mAngularAcceleration.x, 0.01f, 0, FLT_MAX, "%.2f rad/s^2");
		ImGui::TreePop();
	}

	if (ImGui::TreeNodeEx("Input", TREE_NODE_FLAGS))
	{
		ImGui::SliderFloat("Forward Input", &mForwardUserInput, 0, 1);
		ImGui::SliderFloat("Turn Input", &mTurnUserInput, -1, 1);

		ImGui::TreePop();
	}
}