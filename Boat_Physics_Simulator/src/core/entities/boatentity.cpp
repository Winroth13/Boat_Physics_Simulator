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

	PointCloud boatCloud("assets/pointclouds/point_cloud_boat.obj", 1000);
	PointCloud airCloud("assets/pointclouds/point_cloud_air.obj", 0.14f, 1.225f);
	PointCloud engineCloud("assets/pointclouds/point_cloud_engine.obj", 0.14, 7850.f);

	mPointClouds.push_back(boatCloud);
	mPointClouds.push_back(airCloud);
	mPointClouds.push_back(engineCloud);

	mCenterOfMass = CalculateCenterOfMass(mPointClouds);
}

void BoatEntity::UpdateSelf(double deltaTime)
{
	mUpdateTimer += deltaTime;

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

        float inertia = mThrustForce * sinf(turnAngle) * BOAT_LENGTH_TO_MOTOR;
        float momentOfInertia = CalculateMomentOfInertia(BOAT_LENGTH, BOAT_WIDTH, mMass);

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
		BOAT_LENGTH,
		velocityScalar,
		mWaterViscosity
	);

	float cf = CalculateCf(mReynoldsNumber);
	float cr = 0.0f; // We don't care about dynamic waves
	mCh = cf + cr;

	float ratio = (BOAT_HEIGHT / 2.0f - transform.GetPosition3f().y) / BOAT_HEIGHT;
    ratio = ratio < 0.0f ? 0.0f : ratio > 1.0f ? 1.0f : ratio;
	mWaterDragForce = CalculateWaterDragForce(
		WATER_DENSITY,
		(BOAT_WIDTH * BOAT_HEIGHT) * ratio,
		mCh,
        velocityScalar
	);

    float accelerationForwardScalar = (mThrustForce - mWaterDragForce) / mMass;
    acceleration = accelerationForwardScalar * transform.GetForwardDir(); // Forward Thrust Force
    acceleration -= XMVectorSet(0, GRAVITY, 0, 0);

    acceleration += WATER_DENSITY * (BOAT_WIDTH * BOAT_LENGTH * BOAT_HEIGHT * ratio) / mMass * XMVectorSet(0, GRAVITY, 0, 0);

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

	/*for (auto& point : mPointClouds[2].GetPoints())
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
		ImGui::DragFloat("Mass", &mMass, 1, 0.1f, FLT_MAX, "%.2f kg");
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

        int zero = 0;
        static bool matias = false;
        if (ImGui::Checkbox("Matias", &matias))
            if (matias)
                float a = 1 / zero;
	}
}