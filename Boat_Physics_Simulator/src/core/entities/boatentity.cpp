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
	auto vShader = std::make_shared<VertexShader>("resources/VertexShader.cso");
	mSphereModel = std::make_unique<OBJModel>("assets/pointclouds/point_sphere.obj", vShader);

	PointCloud boatCloud("assets/pointclouds/new/point_cloud_boat.obj", 1000);
	PointCloud airCloud("assets/pointclouds/point_cloud_air.obj", 0.14f, 1.225f);
	PointCloud engineCloud("assets/pointclouds/point_cloud_engine.obj", 200);

	boatCloud.SetPointRadius(0.09f);
	engineCloud.SetPointRadius(0.14f);

	mPointClouds.resize((int)PointCloudType::COUNT);

	mPointClouds[static_cast<int>(PointCloudType::BOAT)] = boatCloud;
	mPointClouds[static_cast<int>(PointCloudType::AIR)] = airCloud;
	mPointClouds[static_cast<int>(PointCloudType::ENGINE)] = engineCloud;

	mCenterOfMass = CalculateCenterOfMass(mPointClouds);
	mMass = boatCloud.GetTotalMass() + engineCloud.GetTotalMass();
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

    XMVECTOR velocity = XMLoadFloat3(&mVelocity);
    XMVECTOR acceleration = XMLoadFloat3(&mAcceleration);

    float velocityScalar = XMVectorGetX(XMVector3Length(velocity));

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
    
	mReynoldsNumber = CalculateReynolds(
		WATER_DENSITY,
		GetBoatLength(),
		velocityScalar,
		mWaterViscosity
	);

	float cf = CalculateCf(mReynoldsNumber);
	float cr = 0.0f; // We don't care about dynamic waves
	mCh = cf + cr;

	float ratio = (GetBoatHeight() / 2.0f - transform.GetPosition3f().y) / GetBoatHeight();
    ratio = ratio < 0.0f ? 0.0f : ratio > 1.0f ? 1.0f : ratio;
	mWaterDragForce = CalculateWaterDragForce(
		WATER_DENSITY,
		(GetBoatWidth() * GetBoatHeight() / 2.0f) * ratio,
		mCh,
        velocityScalar
	);

    float accelerationForwardScalar = (mThrustForce - mWaterDragForce) / mMass;
    acceleration = accelerationForwardScalar * transform.GetForwardDir(); // Forward Thrust Force
    acceleration -= XMVectorSet(0, GRAVITY, 0, 0);

	/* Calculate Boat Volumes */
	{
		PointCloud above;
		PointCloud below;

		SplitPointCloud(GetPointCloud(PointCloudType::BOAT), transform.GetMatrix(), 0.0f, above, below);
		mVolumeUnderWater = below.GetVolume();
    }

	/* Calculate Air Volumes */
	{
		PointCloud above;
		PointCloud below;

		SplitPointCloud(GetPointCloud(PointCloudType::AIR), transform.GetMatrix(), 0.0f, above, below);
		mVolumeUnderWater += below.GetVolume();
	}

	/* Calculate Engine Volumes */
	{
		PointCloud above;
		PointCloud below;

		SplitPointCloud(GetPointCloud(PointCloudType::ENGINE), transform.GetMatrix(), 0.0f, above, below);
		mVolumeUnderWater += below.GetVolume();
	}

    acceleration += WATER_DENSITY * mVolumeUnderWater / mMass * XMVectorSet(0, GRAVITY, 0, 0);

    velocity += acceleration * delta;

    XMStoreFloat3(&mVelocity, velocity);
    XMStoreFloat3(&mAcceleration, acceleration);

    transform.MoveX(mVelocity.x * delta);
	transform.MoveY(mVelocity.y * delta);
    transform.MoveZ(mVelocity.z * delta);
}

void BoatEntity::RenderSelf(RenderServer& renderServer)
{
	constexpr float radius = 0.14f;

	Transform pTransform;
	pTransform.SetPosition(mCenterOfMass);
	pTransform.SetScale(radius, radius, radius);

	renderServer.PushMesh(mSphereModel->GetMesh(0), pTransform.GetMatrix() * transform.GetMatrix());
	renderServer.PushMaterial(mSphereModel->GetMaterial(0));

	/*for (auto& point : GetPointCloud(PointCloudType::BOAT).GetPoints())
	{
		Transform pTransform;
		pTransform.SetPosition(point.position);
		pTransform.SetScale(radius, radius, radius);

		renderServer.PushMesh(mSphereModel->GetMesh(0), pTransform.GetMatrix() * transform.GetMatrix());
		renderServer.PushMaterial(mSphereModel->GetMaterial(0));
	}*/
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
	ImGui::Checkbox("Pause", &mPause);

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

    if (ImGui::Button("Start Game")) {
        mPause = false;
        mGameEntity->SetVisible(true);
    }
}