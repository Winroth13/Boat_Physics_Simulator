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

	PointCloud boatCloud("assets/pointclouds/new/point_cloud_boat.obj", 500);
	PointCloud airCloud("assets/pointclouds/new/point_cloud_air.obj", 0.125f, 1.225f);
	PointCloud engineCloud("assets/pointclouds/new/point_cloud_engine.obj", 100);

	boatCloud.SetPointRadius(0.025f);
	engineCloud.SetPointRadius(0.025f);

	mPointClouds.resize((int)PointCloudType::COUNT);

	mPointClouds[static_cast<int>(PointCloudType::BOAT)] = boatCloud;
	mPointClouds[static_cast<int>(PointCloudType::AIR)] = airCloud;
	mPointClouds[static_cast<int>(PointCloudType::ENGINE)] = engineCloud;

	mCenterOfMass = CalculateCenterOfMass(mPointClouds);
	mMass = boatCloud.GetTotalMass() + engineCloud.GetTotalMass();

    // TODO: Calculate y position of equilibrium for the boat
}

void BoatEntity::UpdateSelf(double deltaTime)
{
	if (mPause)
		return;

	mUpdateTimer += static_cast<float>(deltaTime);

	if (mUpdateTimer < UPDATE_RATE)
		return;

	mUpdateTimer -= UPDATE_RATE;
	float delta = UPDATE_RATE;

	Input(delta);

#if 1
    XMVECTOR velocity = XMLoadFloat3(&mVelocity);
    XMVECTOR acceleration = XMLoadFloat3(&mAcceleration);

	/* Calculate Area */
	mFrontAreaUnderWater = mAreaCalculator.CalculateArea(
		mBoatModelEntity->GetModel(),
		transform,
		{0, DirectX::XM_PI, 0}
	);

	mBottomAreaUnderWater = mAreaCalculator.CalculateArea(
		mBoatModelEntity->GetModel(),
		transform,
		{ -DirectX::XM_PIDIV2, 0, 0 }
	);

	/* Calculate Forward Thrust Force */
	{
        XMVECTOR forward = transform.GetForwardDir();
        float velocityForwardScalar = XMVectorGetX(XMVector3Dot(velocity, forward));

        float numerator = (mForwardUserInput * mTotalEfficiency * mEnginePower);
        float denominator = (mHullEfficiency * velocityForwardScalar * (1 - mWakeFactor));
        mThrustForce = numerator / denominator;
	}

    /* Calculate Angular Velocity */
	{
        float turnAngle = GetTurnAngle();

        mMotorHingeEntity->transform.SetYaw(-turnAngle);

		XMFLOAT3 propellerGlobalPosition = mPropellerEntity->GetGlobalPosition();
		propellerGlobalPosition.y = 0.0f;

		XMVECTOR centerOfMass = XMLoadFloat3(&mCenterOfMass);
		centerOfMass = XMVectorSetY(centerOfMass, 0);

		XMVECTOR propellerPosition = XMLoadFloat3(&propellerGlobalPosition);
		XMVECTOR center = XMVector3Transform(centerOfMass, transform.GetMatrix());
		float distanceToCenter = XMVectorGetX(XMVector3Length(propellerPosition - center));

        float inertia = mThrustForce * sinf(turnAngle) * distanceToCenter;
        float momentOfInertia = CalculateMomentOfInertia(GetBoatLength(), GetBoatWidth(), mMass);

        float angularAcceleration = inertia / momentOfInertia;
        mAngularVelocity = angularAcceleration * delta;

        constexpr float A = 500.0f;
        constexpr float k = 6.0f;
        float waterInertia = A * (expf(k * mAngularVelocity) - 1);
        float angularDeceleration = waterInertia / momentOfInertia;

		mAngularVelocity -= angularDeceleration * delta;

		transform.RotateY(mAngularVelocity);
		XMMATRIX rotationMatrix = XMMatrixRotationY(mAngularVelocity);
		velocity = XMVector3Transform(velocity, rotationMatrix);
	}

	mWaterViscosity = CalculateWaterViscosity(WATER_TEMPERATURE, SALT_WATER_CONSTANT);

	/* Calculate forward water drag force */
	{
		XMVECTOR forward = transform.GetForwardDir();
		float velocityForwardScalar = XMVectorGetX(XMVector3Dot(velocity, forward));

		mReynoldsNumber = CalculateReynolds(
			WATER_DENSITY,
			GetBoatLength(),
			velocityForwardScalar,
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
		XMMATRIX rotationMatrix = XMMatrixRotationY(mAngularVelocity);
		waterDragForceV = XMVector3Transform(waterDragForceV, rotationMatrix);
		XMStoreFloat3(&mWaterDragForce, waterDragForceV);
	}

	float accelerationForwardScalar = (mThrustForce - mWaterDragForce.z) / mMass;
	acceleration = accelerationForwardScalar * transform.GetForwardDir(); // Forward Thrust Force

	XMMATRIX transformMatrix = mBoatModelEntity->GetGlobalTransform();

	/* Calculate Boat Volumes */
	{
		PointCloud above;
		PointCloud below;

		SplitPointCloud(GetPointCloud(PointCloudType::BOAT), transformMatrix, 0.0f, above, below);
		mVolumeUnderWater = below.GetVolume();
	}

	/* Calculate Air Volumes */
	{
		PointCloud above;
		PointCloud below;

		SplitPointCloud(GetPointCloud(PointCloudType::AIR), transformMatrix, 0.0f, above, below);
		mVolumeUnderWater += below.GetVolume();
	}

	/* Calculate Engine Volumes */
	{
		PointCloud above;
		PointCloud below;

		SplitPointCloud(GetPointCloud(PointCloudType::ENGINE), transformMatrix, 0.0f, above, below);
		mVolumeUnderWater += below.GetVolume();
	}
    
	switch (mState)
    {
		case BoatState::DEFAULT:
		{
			/* Calculate upwards water drag force */
			{
				float velocityUpScalar = fabsf(XMVectorGetY(velocity));

				mReynoldsNumber = CalculateReynolds(
					WATER_DENSITY,
					GetBoatHeight(),
					velocityUpScalar,
					mWaterViscosity
				);

				float cf = CalculateCf(mReynoldsNumber);
				float cr = 0.0f; // We don't care about dynamic waves
				mCh = cf + cr;

				mWaterDragForce.y = CalculateWaterDragForce(
					WATER_DENSITY,
					mBottomAreaUnderWater,
					mCh,
					velocityUpScalar
				);
			}


			float liftAcceleration = WATER_DENSITY * mVolumeUnderWater / mMass * GRAVITY;
			acceleration += XMVectorSet(0, liftAcceleration + mWaterDragForce.y / mMass - GRAVITY, 0, 0);

			/* 
			*	When Lift Acceleration is almost equal to Gravity, 
			*	force acceleration to move towards 0 to prevent
			*	too much bouncing on the water.
			*/
			if (fabsf(liftAcceleration - GRAVITY) < 0.3f)
			{
				//acceleration *= XMVectorSet(0.0f, 0.01f, 0.0f, 0.0f);
				acceleration *= XMVectorSet(1, 0, 1, 1);
			}

			velocity += acceleration * delta;

			transform.MoveY(XMVectorGetY(velocity) * delta);

			/* Check if we should start ascending */
			if (mVelocity.y < 0.0f && XMVectorGetY(velocity) >= 0.0f)
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
			if (position.y >= -0.05f)
			{
				mState = BoatState::DEFAULT;
				position.y = 0.0f;
			}

            // NEXT: Change position of model / point cloud so that equilibrium is at y = 0 (with some "root" entity or smth idk)

			transform.SetPosition(position);
		}
		break;
	}

	DirectX::XMStoreFloat3(&mVelocity, velocity);
	DirectX::XMStoreFloat3(&mAcceleration, acceleration);

	/* Always move in X and Z */
	transform.MoveX(mVelocity.x * delta);
	transform.MoveZ(mVelocity.z * delta);
#else
	XMMATRIX transformMatrix = mBoatModelEntity->GetGlobalTransform();

	PointCloud boatAbove;
	PointCloud boatBelow;

	PointCloud airAbove;
	PointCloud airBelow;

	PointCloud engineAbove;
	PointCloud engineBelow;

	/* Calculate Boat Volumes */
	{
		SplitPointCloud(GetPointCloud(PointCloudType::BOAT), transformMatrix, 0.0f, boatAbove, boatBelow);
		mVolumeUnderWater = boatBelow.GetVolume();
	}

	/* Calculate Air Volumes */
	{
		SplitPointCloud(GetPointCloud(PointCloudType::AIR), transformMatrix, 0.0f, airAbove, airBelow);
		mVolumeUnderWater += airBelow.GetVolume();
	}

	/* Calculate Engine Volumes */
	{
		SplitPointCloud(GetPointCloud(PointCloudType::ENGINE), transformMatrix, 0.0f, engineAbove, engineBelow);
		mVolumeUnderWater += engineBelow.GetVolume();
	}

	float liftAcceleration = WATER_DENSITY * mVolumeUnderWater / mMass * GRAVITY;
	float gravityAcceleration = GRAVITY;

	Logger::Info(std::to_string(liftAcceleration - gravityAcceleration));

#endif
}

void BoatEntity::RenderSelf(RenderServer& renderServer)
{
	constexpr float radius = 0.025f;

	Transform pTransform;
	pTransform.SetPosition(mCenterOfMass);
	pTransform.SetScale(radius, radius, radius);

	renderServer.PushMesh(mSphereModel->GetMesh(0), pTransform.GetMatrix() * transform.GetMatrix());
	renderServer.PushMaterial(mSphereModel->GetMaterial(0));

	for (auto& point : GetPointCloud(PointCloudType::AIR).GetPoints())
	{
		Transform pTransform;
		pTransform.SetPosition(point.position);
		pTransform.SetScale(radius, radius, radius);

		renderServer.PushMesh(mSphereModel->GetMesh(0), pTransform.GetMatrix() * mBoatModelEntity->GetGlobalTransform());
		renderServer.PushMaterial(mSphereModel->GetMaterial(0));
	}
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

void BoatEntity::RenderImguiSelf()
{
    if (ImGui::Button("Start Game")) {
        mPause = false;
        mGameEntity->SetVisible(true);
    }

	ImGui::Checkbox("Pause", &mPause);

	ImGui::Text("State: %s", BoatStateToString(mState).c_str());

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

		ImGui::TextColored(ImVec4(1, 0, 0, 1), "Water Drag Force: ", mWaterDragForce.z);
		ImGui::SameLine();
		ImGui::TextColored(ImVec4(0, 1, 0, 1), "Y: %.2f ", mWaterDragForce.y);
		ImGui::SameLine();
		ImGui::TextColored(ImVec4(0, 0, 1, 1), "Z: %.2f", mWaterDragForce.z);

		ImGui::TreePop();
	}

	if (ImGui::TreeNodeEx("Constants", TREE_NODE_FLAGS))
	{
		ImGui::Text("Boat Width: %.2f m", GetBoatWidth());
		ImGui::Text("Boat Height: %.2f m", GetBoatHeight());
		ImGui::Text("Boat Length: %.2f m", GetBoatLength());

		ImGui::Text("Boat Mass: %.2f kg", mMass);

		float totalVolume = 0;
		totalVolume += GetPointCloud(PointCloudType::BOAT).GetVolume();
		totalVolume += GetPointCloud(PointCloudType::AIR).GetVolume();
		totalVolume += GetPointCloud(PointCloudType::ENGINE).GetVolume();
		ImGui::Text("Volume: %.2f m^3", totalVolume);

		float percentBelowWater = (mVolumeUnderWater / totalVolume) * 100;

		ImGui::Text("Volume Below Water: %.1f %%", percentBelowWater);
		ImGui::Text("Volume Above Water: %.1f %%", 100 - percentBelowWater);

		ImGui::Text("Front Area Below Water: %.3f m^2", mFrontAreaUnderWater);
		ImGui::Text("Bottom Area Below Water: %.3f m^2", mBottomAreaUnderWater);

		ImGui::DragFloat("Wake Factor", &mWakeFactor, 0.001f, 0, 0.99f);
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
		ImGui::DragFloat3("Velocity", &mVelocity.x, 0.01f, 0, FLT_MAX, "%.1f m/s");
		ImGui::DragFloat3("Acceleration", &mAcceleration.x, 0.01f, 0, FLT_MAX, "%.1f m/s^2");

		ImGui::DragFloat("Angular Velocity", &mAngularVelocity, 0.01f, 0, FLT_MAX, "%.1f rad/s");
		ImGui::TreePop();
	}
}