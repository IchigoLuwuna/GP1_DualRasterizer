#include <bit>
#include "Sampler.h"
#include "Error.h"

Sampler::Sampler( ID3D11Device* pDevice, ID3DX11Effect* pEffect )
{
	m_pSamplerVariable = pEffect->GetVariableByName( "gSampler" )->AsSampler();

	if ( !m_pSamplerVariable )
	{
		throw error::effect::InvalidSampler();
	}

	HRESULT result{};

	D3D11_SAMPLER_DESC samplerDesc{};
	samplerDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_POINT;
	samplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
	samplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
	samplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;

	result = pDevice->CreateSamplerState( &samplerDesc, &m_pPointSampler );
	if ( FAILED( result ) )
	{
		throw error::effect::InvalidSampler();
	}

	samplerDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
	result = pDevice->CreateSamplerState( &samplerDesc, &m_pLinearSampler );
	if ( FAILED( result ) )
	{
		throw error::effect::InvalidSampler();
	}

	samplerDesc.Filter = D3D11_FILTER_ANISOTROPIC;
	result = pDevice->CreateSamplerState( &samplerDesc, &m_pAnisotropicSampler );
	if ( FAILED( result ) )
	{
		throw error::effect::InvalidSampler();
	}

	Update();
}

Sampler::Sampler( Sampler&& rhs )
{
	if ( this == &rhs )
	{
		return;
	}

	m_pSamplerVariable = rhs.m_pSamplerVariable;
	rhs.m_pSamplerVariable = nullptr;

	m_pPointSampler = rhs.m_pPointSampler;
	rhs.m_pPointSampler = nullptr;

	m_pLinearSampler = rhs.m_pLinearSampler;
	rhs.m_pLinearSampler = nullptr;

	m_pAnisotropicSampler = rhs.m_pAnisotropicSampler;
	rhs.m_pAnisotropicSampler = nullptr;
}

Sampler& Sampler::operator=( Sampler&& rhs )
{
	if ( this == &rhs )
	{
		return *this;
	}

	m_pSamplerVariable = rhs.m_pSamplerVariable;
	rhs.m_pSamplerVariable = nullptr;

	m_pPointSampler = rhs.m_pPointSampler;
	rhs.m_pPointSampler = nullptr;

	m_pLinearSampler = rhs.m_pLinearSampler;
	rhs.m_pLinearSampler = nullptr;

	m_pAnisotropicSampler = rhs.m_pAnisotropicSampler;
	rhs.m_pAnisotropicSampler = nullptr;

	return *this;
}

Sampler::~Sampler()
{
	if ( m_pSamplerVariable )
	{
		m_pSamplerVariable->Release();
	}

	if ( m_pPointSampler )
	{
		m_pPointSampler->Release();
	}

	if ( m_pLinearSampler )
	{
		m_pLinearSampler->Release();
	}

	if ( m_pAnisotropicSampler )
	{
		m_pAnisotropicSampler->Release();
	}
}

void Sampler::Cycle()
{
	IncrementFilterMode();

	Update();
}

void Sampler::Update()
{
	switch ( m_CurrentFilterMode )
	{
	case FilterMode::point:
		std::cout << "Set filter mode to Point\n";
		m_pSamplerVariable->SetSampler( 0, m_pPointSampler );
		break;

	case FilterMode::linear:
		std::cout << "Set filter mode to Linear\n";
		m_pSamplerVariable->SetSampler( 0, m_pLinearSampler );
		break;

	case FilterMode::anisotropic:
		std::cout << "Set filter mode to Anisotropic\n";
		m_pSamplerVariable->SetSampler( 0, m_pAnisotropicSampler );
		break;

	default:
		break;
	}
}

void Sampler::IncrementFilterMode()
{
	m_CurrentFilterMode =
		std::bit_cast<FilterMode, int>( ( std::bit_cast<int, FilterMode>( m_CurrentFilterMode ) + 1 ) %
										std::bit_cast<int, FilterMode>( FilterMode::count ) );
}
