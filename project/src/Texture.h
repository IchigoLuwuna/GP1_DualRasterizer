#ifndef TEXTURE_H
#define TEXTURE_H
#include <string>
#include <SDL_surface.h>
#include <d3d11.h>
#include "Structs.h"

namespace dae
{
class Texture
{
public:
	Texture() = default;
	Texture( ID3D11Device* pDevice, const std::string& texturePath );
	Texture( const Texture& ) = delete;
	Texture( Texture&& rhs );
	~Texture() noexcept;

	Texture& operator=( const Texture& ) = delete;
	Texture& operator=( Texture&& rhs );

	ID3D11ShaderResourceView* GetSRV() const;

	// Software Rendering
	ColorRGB Sample( const Vector2& uv ) const;
	//

private:
	ID3D11Texture2D* m_pResource{};
	ID3D11ShaderResourceView* m_pResourceView{};

	// Software rendering
	SDL_Surface* m_pSurface{};
	//
};
} // namespace dae
#endif
