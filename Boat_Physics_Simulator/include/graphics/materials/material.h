#pragma once
#include "core/renderer/renderserver.h"

#include "graphics/shaders/vertexshader.h"
#include "graphics/textures/texture2d.h"

#include <DirectXMath.h>
#include <d3d11.h>

enum class MaterialFlags : uint32_t
{
	NONE = 0,
	HAS_NORMAL_MAP = 1,
	HAS_DISPLACEMENT_MAP = 2,
	HAS_ALPHA_MAP = 4
};

class Material
{
public:
	Material(
		std::shared_ptr<VertexShader> vertexShader,
		std::shared_ptr<Texture2D> texture2D
	);
	virtual ~Material();

	inline std::shared_ptr<VertexShader> GetVertexShader() { return mVertexShader; }
	inline ID3D11InputLayout* GetInputLayout() { return mInputLayout; }

	inline std::shared_ptr<Texture2D> GetTexture() { return mTexture; }
	inline std::shared_ptr<Texture2D> GetNormalMap() { return mNormalMap; }
	inline std::shared_ptr<CubemapTexture> GetCubemapTexture() { return mCubemapTexture; }
	inline std::shared_ptr<Texture2D> GetDisplacementMap() { return mDisplacementMap; }
	inline std::shared_ptr<Texture2D> GetAlphaMap() { return mAlphaMap; }

	void SetAmbientCoefficient(const float r, const float g, const float b);
	void SetDiffuseCoefficient(const float r, const float g, const float b);
	void SetSpecularCoefficient(const float r, const float g, const float b);
	void SetPhongExponent(const float phongExponent);
	void SetReflectiveness(const float reflectiveness);
	void SetMaxTessFactor(const float maxTessFactor);
	void SetMaxTessDistance(const float maxTessDistance);
	void SetMinTessDistance(const float minTessDistance);
	void SetDispStrength(const float dispSttrength);

	void SetCubemapTexture(std::shared_ptr<CubemapTexture> cubemapTexture) { mCubemapTexture = cubemapTexture; }
	void SetNormalMap(std::shared_ptr<Texture2D> normalMap);
	void SetDisplacementMap(std::shared_ptr<Texture2D> displacementMap);
	void SetAlphaMap(std::shared_ptr<Texture2D> alphaMap);

	void RenderImgui();

	inline DirectX::XMVECTOR GetAmbientCoefficient() { return DirectX::XMLoadFloat3(&mAmbientCoefficient); }
	inline DirectX::XMVECTOR GetDiffuseCoefficient() { return DirectX::XMLoadFloat3(&mDiffuseCoefficient); }
	inline DirectX::XMVECTOR GetSpecularCoefficient() { return DirectX::XMLoadFloat3(&mSpecularCoefficient); }

	inline DirectX::XMFLOAT3& GetAmbientCoefficient3f() { return mAmbientCoefficient; }
	inline DirectX::XMFLOAT3& GetDiffuseCoefficient3f() { return mDiffuseCoefficient; }
	inline DirectX::XMFLOAT3& GetSpecularCoefficient3f() { return mSpecularCoefficient; }

	inline float& GetPhongExponent() { return mPhongExponent; }
	inline float& GetReflectiveness() { return mReflectiveness; }
	inline float& GetMaxTessFactor() { return mMaxTessFactor; }
	inline float& GetMaxTessDistance() { return mMaxTessDistance; }
	inline float& GetMinTessDistance() { return mMinTessDistance; }
	inline float& SetDispStrength() { return mDispStrength; }

	bool HasAlpha() const;
	bool HasDisplacement() const;

	inline uint32_t GetFlags() { return mFlags; }

	void SetName(const std::string& name) { mName = name; }
	const std::string& GetName() { return mName; }

private:
	bool mIsDirty = true;
	float mPhongExponent = 8;
	float mReflectiveness = 0;
	float mMaxTessFactor = 1.0f;
	float mMaxTessDistance = 0.0f;
	float mMinTessDistance = 1.0f;
	float mDispStrength = 0.05f;

	std::string mName;

	DirectX::XMFLOAT3 mAmbientCoefficient = { 1, 1, 1 };
	DirectX::XMFLOAT3 mDiffuseCoefficient = { 1, 1, 1 };
	DirectX::XMFLOAT3 mSpecularCoefficient = { 1, 1, 1 };

	std::shared_ptr<VertexShader> mVertexShader = nullptr;
	std::shared_ptr<Texture2D> mTexture = nullptr;
	std::shared_ptr<CubemapTexture> mCubemapTexture = nullptr;
	std::shared_ptr<Texture2D> mNormalMap = nullptr;
	std::shared_ptr<Texture2D> mDisplacementMap = nullptr;
	std::shared_ptr<Texture2D> mAlphaMap = nullptr;

	uint32_t mFlags = 0;

	ID3D11InputLayout* mInputLayout = nullptr;
};