#include "Texture.h"
#include <SDL_image.h>
#include "Error.h"

namespace dae
{
Texture::Texture( ID3D11Device* pDevice, const std::string& texturePath )
{
	HRESULT result{};

	m_pSurface = IMG_Load( texturePath.c_str() );

	DXGI_FORMAT format{ DXGI_FORMAT_R8G8B8A8_UNORM };
	D3D11_TEXTURE2D_DESC desc{};
	desc.Width = m_pSurface->w;
	desc.Height = m_pSurface->h;
	desc.MipLevels = 1;
	desc.ArraySize = 1;
	desc.Format = format;
	desc.SampleDesc.Count = 1;
	desc.SampleDesc.Quality = 0;
	desc.Usage = D3D11_USAGE_DEFAULT;
	desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;

	D3D11_SUBRESOURCE_DATA texData{};
	texData.pSysMem = m_pSurface->pixels;
	texData.SysMemPitch = static_cast<UINT>( m_pSurface->pitch );
	texData.SysMemSlicePitch = static_cast<UINT>( m_pSurface->h * m_pSurface->pitch );

	result = pDevice->CreateTexture2D( &desc, &texData, &m_pResource );
	if ( FAILED( result ) )
	{
		throw error::texture::ResourceCreateFail();
	}

	D3D11_SHADER_RESOURCE_VIEW_DESC resourceViewDesc{};
	resourceViewDesc.Format = format;
	resourceViewDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
	resourceViewDesc.Texture2D.MipLevels = 1;

	result = pDevice->CreateShaderResourceView( m_pResource, &resourceViewDesc, &m_pResourceView );
	if ( FAILED( result ) )
	{
		throw error::texture::ResourceViewCreateFail();
	}
}

Texture::Texture( Texture&& rhs )
{
	if ( this == &rhs )
	{
		return;
	}

	m_pResource = rhs.m_pResource;
	rhs.m_pResource = nullptr;

	m_pResourceView = rhs.m_pResourceView;
	rhs.m_pResourceView = nullptr;

	m_pSurface = rhs.m_pSurface;
	rhs.m_pSurface = nullptr;
}

Texture& Texture::operator=( Texture&& rhs )
{
	if ( this == &rhs )
	{
		return *this;
	}

	m_pResource = rhs.m_pResource;
	rhs.m_pResource = nullptr;

	m_pResourceView = rhs.m_pResourceView;
	rhs.m_pResourceView = nullptr;

	m_pSurface = rhs.m_pSurface;
	rhs.m_pSurface = nullptr;

	return *this;
}

Texture::~Texture() noexcept
{
	if ( m_pResourceView )
	{
		m_pResourceView->Release();
	}

	if ( m_pResource )
	{
		m_pResource->Release();
	}

	if ( m_pSurface )
	{
		SDL_FreeSurface( m_pSurface );
	}
}

ID3D11ShaderResourceView* Texture::GetSRV() const
{
	return m_pResourceView;
}

ColorRGB Texture::Sample( const Vector2& uv ) const
{
	uint32_t* surfacePixels{ reinterpret_cast<uint32_t*>( m_pSurface->pixels ) };

	// Convert UV coordinates to texture coordinates
	const int pixelWidth{ m_pSurface->w };
	const int pixelHeight{ m_pSurface->h };
	const int uvpx{ static_cast<int>( std::round( uv.x * pixelWidth ) ) };
	const int uvpy{ static_cast<int>( std::round( uv.y * pixelHeight ) ) };

	const int pixelIndex{ uvpx + uvpy * pixelWidth };
	uint32_t pixel{ surfacePixels[pixelIndex] };
	const uint8_t pixelR{ *reinterpret_cast<const uint8_t*>( &pixel ) }; // Capture first 8 bits -> Red
	pixel = pixel >> 8;													 // Shift right to capture next
	const uint8_t pixelG{ *reinterpret_cast<const uint8_t*>( &pixel ) }; // Capture first 8 bits -> Green
	pixel = pixel >> 8;													 // Shift right to capture next
	const uint8_t pixelB{ *reinterpret_cast<const uint8_t*>( &pixel ) }; // Capture first 8 bits -> Blue

	// Convert unsigned int RGB to float RGB
	const ColorRGB color{ pixelR / 255.f, pixelG / 255.f, pixelB / 255.f };
	return color;
}
} // namespace dae
