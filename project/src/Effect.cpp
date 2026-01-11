#include <sstream>
#include <array>
#include <d3dx11effect.h>
#include "Effect.h"
#include "Error.h"

using namespace dae;

Effect::Effect( ID3D11Device* pDevice, const std::wstring& assetFile )
{
	m_pEffect = Effect::LoadEffect( pDevice, assetFile );

	if ( !m_pEffect )
	{
		throw error::effect::CreateFail();
	}

	if ( !m_pEffect->IsValid() )
	{
		throw error::effect::InvalidEffect();
	}

	m_pTechnique = m_pEffect->GetTechniqueByName( "DefaultTechnique" );

	if ( !m_pTechnique->IsValid() )
	{
		throw error::effect::InvalidTechnique();
	}

	// Create Vertex Layout
	constexpr int elementCount{ 5 };
	std::array<D3D11_INPUT_ELEMENT_DESC, elementCount> vertexDesc{};

	vertexDesc[0].SemanticName = "POSITION";
	vertexDesc[0].Format = DXGI_FORMAT_R32G32B32_FLOAT;
	vertexDesc[0].AlignedByteOffset = 0;
	vertexDesc[0].InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA;

	vertexDesc[1].SemanticName = "COLOR";
	vertexDesc[1].Format = DXGI_FORMAT_R32G32B32_FLOAT;
	vertexDesc[1].AlignedByteOffset = 12;
	vertexDesc[1].InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA;

	vertexDesc[2].SemanticName = "TEXCOORD";
	vertexDesc[2].Format = DXGI_FORMAT_R32G32_FLOAT;
	vertexDesc[2].AlignedByteOffset = 24;
	vertexDesc[2].InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA;

	vertexDesc[3].SemanticName = "NORMAL";
	vertexDesc[3].Format = DXGI_FORMAT_R32G32B32_FLOAT;
	vertexDesc[3].AlignedByteOffset = 32;
	vertexDesc[3].InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA;

	vertexDesc[4].SemanticName = "TANGENT";
	vertexDesc[4].Format = DXGI_FORMAT_R32G32B32_FLOAT;
	vertexDesc[4].AlignedByteOffset = 44;
	vertexDesc[4].InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA;
	//

	// Create Input Layout
	D3DX11_PASS_DESC passDesc{};
	m_pTechnique->GetPassByIndex( 0 )->GetDesc( &passDesc );

	HRESULT result{ pDevice->CreateInputLayout( vertexDesc.data(),
												vertexDesc.size(),
												passDesc.pIAInputSignature,
												passDesc.IAInputSignatureSize,
												&m_pInputLayout ) };
	if ( FAILED( result ) )
	{
		throw error::effect::LayoutCreateFail();
	}
	//

	// Get pointers to shader variables
	m_pWorldViewProjection = m_pEffect->GetVariableByName( "gWorldViewProj" )->AsMatrix();
	if ( !m_pWorldViewProjection->IsValid() )
	{
		throw error::effect::InvalidWorldViewProjection();
	}

	m_pWorld = m_pEffect->GetVariableByName( "gWorld" )->AsMatrix();
	if ( !m_pWorld->IsValid() )
	{
		throw error::effect::InvalidWorld();
	}

	m_pCameraOrigin = m_pEffect->GetVariableByName( "gCameraOrigin" )->AsVector();
	if ( !m_pCameraOrigin->IsValid() )
	{
		throw error::effect::InvalidCameraOrigin();
	}

	m_pDiffuseMap = m_pEffect->GetVariableByName( "gDiffuseMap" )->AsShaderResource();
	if ( !m_pDiffuseMap->IsValid() )
	{
		throw error::effect::InvalidMap();
	}

	m_pNormalMap = m_pEffect->GetVariableByName( "gNormalMap" )->AsShaderResource();
	if ( !m_pNormalMap->IsValid() )
	{
		throw error::effect::InvalidMap();
	}

	m_pSpecularMap = m_pEffect->GetVariableByName( "gSpecularMap" )->AsShaderResource();
	if ( !m_pSpecularMap->IsValid() )
	{
		throw error::effect::InvalidMap();
	}

	m_pGlossMap = m_pEffect->GetVariableByName( "gGlossMap" )->AsShaderResource();
	if ( !m_pGlossMap->IsValid() )
	{
		throw error::effect::InvalidMap();
	}
	//

	// Create the sampler state
	m_Sampler = Sampler( pDevice, m_pEffect );
	//
}

Effect::Effect( Effect&& rhs )
{
	if ( this == &rhs )
	{
		return;
	}

	// OWNING
	m_pEffect = rhs.m_pEffect;
	rhs.m_pEffect = nullptr;

	m_pInputLayout = rhs.m_pInputLayout;
	rhs.m_pInputLayout = nullptr;

	m_Sampler = std::move( rhs.m_Sampler );
	//

	// NON-OWNING
	m_pTechnique = rhs.m_pTechnique;
	rhs.m_pTechnique = nullptr;

	m_pWorldViewProjection = rhs.m_pWorldViewProjection;
	rhs.m_pWorldViewProjection = nullptr;

	m_pWorld = rhs.m_pWorld;
	rhs.m_pWorld = nullptr;

	m_pCameraOrigin = rhs.m_pCameraOrigin;
	rhs.m_pCameraOrigin = nullptr;

	m_pDiffuseMap = rhs.m_pDiffuseMap;
	rhs.m_pDiffuseMap = nullptr;

	m_pNormalMap = rhs.m_pNormalMap;
	rhs.m_pNormalMap = nullptr;

	m_pSpecularMap = rhs.m_pSpecularMap;
	rhs.m_pSpecularMap = nullptr;

	m_pGlossMap = rhs.m_pGlossMap;
	rhs.m_pSpecularMap = nullptr;
	//
}

Effect& Effect::operator=( Effect&& rhs )
{
	if ( this == &rhs )
	{
		return *this;
	}

	// OWNING
	m_pEffect = rhs.m_pEffect;
	rhs.m_pEffect = nullptr;

	m_pInputLayout = rhs.m_pInputLayout;
	rhs.m_pInputLayout = nullptr;

	m_Sampler = std::move( rhs.m_Sampler );
	//

	// NON-OWNING
	m_pTechnique = rhs.m_pTechnique;
	rhs.m_pTechnique = nullptr;

	m_pWorldViewProjection = rhs.m_pWorldViewProjection;
	rhs.m_pWorldViewProjection = nullptr;

	m_pWorld = rhs.m_pWorld;
	rhs.m_pWorld = nullptr;

	m_pCameraOrigin = rhs.m_pCameraOrigin;
	rhs.m_pCameraOrigin = nullptr;

	m_pDiffuseMap = rhs.m_pDiffuseMap;
	rhs.m_pDiffuseMap = nullptr;

	m_pNormalMap = rhs.m_pNormalMap;
	rhs.m_pNormalMap = nullptr;

	m_pSpecularMap = rhs.m_pSpecularMap;
	rhs.m_pSpecularMap = nullptr;

	m_pGlossMap = rhs.m_pGlossMap;
	rhs.m_pSpecularMap = nullptr;
	//

	return *this;
}

Effect::~Effect() noexcept
{
	if ( m_pEffect )
	{
		m_pEffect->Release();
	}

	if ( m_pInputLayout )
	{
		m_pInputLayout->Release();
	}
}

ID3DX11Effect* Effect::operator->()
{
	return m_pEffect;
}

void Effect::CycleFilteringMode()
{
	m_Sampler.Cycle();
}

void Effect::SetWorldViewProjection( const Matrix& wvp )
{
	m_pWorldViewProjection->SetMatrix( reinterpret_cast<const float*>( &wvp ) );
}

void Effect::SetWorld( const Matrix& w )
{
	m_pWorld->SetMatrix( reinterpret_cast<const float*>( &w ) );
}

void Effect::SetCameraOrigin( const Vector3& o )
{
	const Vector4 input{ o, 1.f };
	m_pCameraOrigin->SetFloatVector( reinterpret_cast<const float*>( &input ) );
}

void Effect::SetDiffuseMap( const Texture& diffuseMap )
{
	m_pDiffuseMap->SetResource( diffuseMap.GetSRV() );
}

void Effect::SetNormalMap( const Texture& normalMap )
{
	m_pNormalMap->SetResource( normalMap.GetSRV() );
}

void Effect::SetSpecularMap( const Texture& specularMap )
{
	m_pSpecularMap->SetResource( specularMap.GetSRV() );
}

void Effect::SetGlossMap( const Texture& glossMap )
{
	m_pGlossMap->SetResource( glossMap.GetSRV() );
}

ID3DX11EffectTechnique* Effect::GetTechniquePtr() const
{
	return m_pTechnique;
}

ID3D11InputLayout* Effect::GetInputLayoutPtr() const
{
	return m_pInputLayout;
}

TransparentEffect::TransparentEffect( ID3D11Device* pDevice, const std::wstring& assetFile )
{
	m_pEffect = Effect::LoadEffect( pDevice, assetFile );

	if ( !m_pEffect )
	{
		throw error::effect::CreateFail();
	}

	if ( !m_pEffect->IsValid() )
	{
		throw error::effect::InvalidEffect();
	}

	m_pTechnique = m_pEffect->GetTechniqueByName( "DefaultTechnique" );

	if ( !m_pTechnique->IsValid() )
	{
		throw error::effect::InvalidTechnique();
	}

	// Create Vertex Layout
	constexpr int elementCount{ 5 };
	std::array<D3D11_INPUT_ELEMENT_DESC, elementCount> vertexDesc{};

	vertexDesc[0].SemanticName = "POSITION";
	vertexDesc[0].Format = DXGI_FORMAT_R32G32B32_FLOAT;
	vertexDesc[0].AlignedByteOffset = 0;
	vertexDesc[0].InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA;

	vertexDesc[1].SemanticName = "COLOR";
	vertexDesc[1].Format = DXGI_FORMAT_R32G32B32_FLOAT;
	vertexDesc[1].AlignedByteOffset = 12;
	vertexDesc[1].InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA;

	vertexDesc[2].SemanticName = "TEXCOORD";
	vertexDesc[2].Format = DXGI_FORMAT_R32G32_FLOAT;
	vertexDesc[2].AlignedByteOffset = 24;
	vertexDesc[2].InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA;

	vertexDesc[3].SemanticName = "NORMAL";
	vertexDesc[3].Format = DXGI_FORMAT_R32G32B32_FLOAT;
	vertexDesc[3].AlignedByteOffset = 32;
	vertexDesc[3].InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA;

	vertexDesc[4].SemanticName = "TANGENT";
	vertexDesc[4].Format = DXGI_FORMAT_R32G32B32_FLOAT;
	vertexDesc[4].AlignedByteOffset = 44;
	vertexDesc[4].InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA;
	//

	// Create Input Layout
	D3DX11_PASS_DESC passDesc{};
	m_pTechnique->GetPassByIndex( 0 )->GetDesc( &passDesc );

	HRESULT result{ pDevice->CreateInputLayout( vertexDesc.data(),
												vertexDesc.size(),
												passDesc.pIAInputSignature,
												passDesc.IAInputSignatureSize,
												&m_pInputLayout ) };
	if ( FAILED( result ) )
	{
		throw error::effect::LayoutCreateFail();
	}
	//

	// Get pointers to shader variables
	m_pWorldViewProjection = m_pEffect->GetVariableByName( "gWorldViewProj" )->AsMatrix();
	if ( !m_pWorldViewProjection->IsValid() )
	{
		throw error::effect::InvalidWorldViewProjection();
	}

	m_pDiffuseMap = m_pEffect->GetVariableByName( "gDiffuseMap" )->AsShaderResource();
	if ( !m_pDiffuseMap->IsValid() )
	{
		throw error::effect::InvalidMap();
	}
	//

	// Create the sampler state
	m_Sampler = Sampler( pDevice, m_pEffect );
	//
}

TransparentEffect::TransparentEffect( TransparentEffect&& rhs )
{
	if ( this == &rhs )
	{
		return;
	}

	// OWNING
	m_pEffect = rhs.m_pEffect;
	rhs.m_pEffect = nullptr;

	m_pInputLayout = rhs.m_pInputLayout;
	rhs.m_pInputLayout = nullptr;

	m_Sampler = std::move( rhs.m_Sampler );
	//

	// NON-OWNING
	m_pTechnique = rhs.m_pTechnique;
	rhs.m_pTechnique = nullptr;

	m_pWorldViewProjection = rhs.m_pWorldViewProjection;
	rhs.m_pWorldViewProjection = nullptr;

	m_pDiffuseMap = rhs.m_pDiffuseMap;
	rhs.m_pDiffuseMap = nullptr;
	//
}

TransparentEffect& TransparentEffect::operator=( TransparentEffect&& rhs )
{
	if ( this == &rhs )
	{
		return *this;
	}

	// OWNING
	m_pEffect = rhs.m_pEffect;
	rhs.m_pEffect = nullptr;

	m_pInputLayout = rhs.m_pInputLayout;
	rhs.m_pInputLayout = nullptr;

	m_Sampler = std::move( rhs.m_Sampler );
	//

	// NON-OWNING
	m_pTechnique = rhs.m_pTechnique;
	rhs.m_pTechnique = nullptr;

	m_pWorldViewProjection = rhs.m_pWorldViewProjection;
	rhs.m_pWorldViewProjection = nullptr;

	m_pDiffuseMap = rhs.m_pDiffuseMap;
	rhs.m_pDiffuseMap = nullptr;
	//

	return *this;
}

TransparentEffect::~TransparentEffect() noexcept
{
	if ( m_pEffect )
	{
		m_pEffect->Release();
	}

	if ( m_pInputLayout )
	{
		m_pInputLayout->Release();
	}
}

ID3DX11Effect* TransparentEffect::operator->()
{
	return m_pEffect;
}

void TransparentEffect::CycleFilteringMode()
{
	m_Sampler.Cycle();
}

void TransparentEffect::SetWorldViewProjection( const Matrix& wvp )
{
	m_pWorldViewProjection->SetMatrix( reinterpret_cast<const float*>( &wvp ) );
}

void TransparentEffect::SetDiffuseMap( const Texture& diffuseMap )
{
	m_pDiffuseMap->SetResource( diffuseMap.GetSRV() );
}

ID3DX11EffectTechnique* TransparentEffect::GetTechniquePtr() const
{
	return m_pTechnique;
}

ID3D11InputLayout* TransparentEffect::GetInputLayoutPtr() const
{
	return m_pInputLayout;
}

ID3DX11Effect* Effect::LoadEffect( ID3D11Device* pDevice, const std::wstring& assetFile )
{
	HRESULT result{};
	ID3D10Blob* pErrorBlob{};
	ID3DX11Effect* pEffect{};

	DWORD shaderFlags{};
#if defined( DEBUG ) || defined( _DEBUG )
	shaderFlags |= D3DCOMPILE_DEBUG;
	shaderFlags |= D3DCOMPILE_SKIP_OPTIMIZATION;
#endif
	result = D3DX11CompileEffectFromFile(
		assetFile.c_str(), nullptr, nullptr, shaderFlags, 0, pDevice, &pEffect, &pErrorBlob );

	if ( FAILED( result ) )
	{
		if ( pErrorBlob )
		{
			const char* pErrors{ reinterpret_cast<char*>( pErrorBlob->GetBufferPointer() ) };

			std::wstringstream stringStream{};
			for ( size_t index{}; index < pErrorBlob->GetBufferSize(); ++index )
			{
				stringStream << pErrors[index];
			}

			OutputDebugStringW( stringStream.str().c_str() );
			pErrorBlob->Release();
			pErrorBlob = nullptr;

			std::wcout << stringStream.str() << "\n";

			return nullptr;
		}
		else
		{
			std::wcout << L"EffectLoader: Failed to CreateEffectFromFile!\nPath: " << assetFile << "\n";
			return nullptr;
		}
	}

	return pEffect;
}
