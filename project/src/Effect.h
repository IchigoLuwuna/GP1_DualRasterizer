#ifndef EFFECT_H
#define EFFECT_H

// This is an RAII wrapper around DirectX effects
// Project includes
#include "Matrix.h"
#include "Sampler.h"
#include "Texture.h"

namespace dae
{

class Effect final
{
public:
	Effect() = default;
	Effect( ID3D11Device* pDevice, const std::wstring& assetFile );
	Effect( const Effect& ) = delete;
	Effect( Effect&& rhs );
	Effect& operator=( const Effect& ) = delete;
	Effect& operator=( Effect&& rhs );

	~Effect() noexcept;

	ID3DX11Effect* operator->(); // Access effect

	// Methods
	void CycleFilteringMode();

	// Setters
	void SetWorldViewProjection( const Matrix& wvp );
	void SetWorld( const Matrix& w );
	void SetCameraOrigin( const Vector3& o );
	void SetDiffuseMap( const Texture& diffuseMap );
	void SetNormalMap( const Texture& normalMap );
	void SetSpecularMap( const Texture& specularMap );
	void SetGlossMap( const Texture& glossMap );

	// Getters
	ID3DX11EffectTechnique* GetTechniquePtr() const;
	ID3D11InputLayout* GetInputLayoutPtr() const;

	static ID3DX11Effect* LoadEffect( ID3D11Device* pDevice, const std::wstring& assetFile );

private:
	// HARDWARE RESOURCES: OWNING
	ID3DX11Effect* m_pEffect{};
	ID3D11InputLayout* m_pInputLayout{};
	Sampler m_Sampler{};
	//

	// HARDWARE RESOURCES: NON-OWNING
	ID3DX11EffectTechnique* m_pTechnique{};
	ID3DX11EffectMatrixVariable* m_pWorldViewProjection{};
	ID3DX11EffectMatrixVariable* m_pWorld{};
	ID3DX11EffectVectorVariable* m_pCameraOrigin{};
	ID3DX11EffectShaderResourceVariable* m_pDiffuseMap{};
	ID3DX11EffectShaderResourceVariable* m_pNormalMap{};
	ID3DX11EffectShaderResourceVariable* m_pSpecularMap{};
	ID3DX11EffectShaderResourceVariable* m_pGlossMap{};
	//
};

class TransparentEffect final // see transparent mesh
{
public:
	TransparentEffect() = default;
	TransparentEffect( ID3D11Device* pDevice, const std::wstring& assetFile );
	TransparentEffect( const Effect& ) = delete;
	TransparentEffect( TransparentEffect&& rhs );
	TransparentEffect& operator=( const TransparentEffect& ) = delete;
	TransparentEffect& operator=( TransparentEffect&& rhs );

	~TransparentEffect() noexcept;

	ID3DX11Effect* operator->(); // Access effect

	// Methods
	void CycleFilteringMode();

	// Setters
	void SetWorldViewProjection( const Matrix& wvp );
	void SetDiffuseMap( const Texture& diffuseMap );

	// Getters
	ID3DX11EffectTechnique* GetTechniquePtr() const;
	ID3D11InputLayout* GetInputLayoutPtr() const;

private:
	// HARDWARE RESOURCES: OWNING
	ID3DX11Effect* m_pEffect{};
	ID3D11InputLayout* m_pInputLayout{};
	Sampler m_Sampler{};
	//

	// HARDWARE RESOURCES: NON-OWNING
	ID3DX11EffectTechnique* m_pTechnique{};
	ID3DX11EffectMatrixVariable* m_pWorldViewProjection{};
	ID3DX11EffectShaderResourceVariable* m_pDiffuseMap{};
	//
};
} // namespace dae
#endif
