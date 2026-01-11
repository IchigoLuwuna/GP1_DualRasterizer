#ifndef HARDWARE_RENDERER_H
#define HARDWARE_RENDERER_H

// SDL Headers
#include "SDL.h"
#include "Effect.h"

// DirectX Headers
#include <dxgi.h>
#include <d3d11.h>
#include <d3dcompiler.h>
#include <d3dx11effect.h>

// Framework Headers
#include "Timer.h"
#include "Scene.h"

namespace dae
{
class HardwareRenderer final
{
public:
	HardwareRenderer( SDL_Window* pWindow );
	~HardwareRenderer() noexcept;

	HardwareRenderer( const HardwareRenderer& ) = delete;
	HardwareRenderer( HardwareRenderer&& ) noexcept = delete;
	HardwareRenderer& operator=( const HardwareRenderer& ) = delete;
	HardwareRenderer& operator=( HardwareRenderer&& ) noexcept = delete;

	void Update( const Timer& timer );
	void Render( Scene* pScene );

	void InitScene( Scene* pScene );

private:
	int m_Width{};
	int m_Height{};

	bool m_IsInitialized{ false };

	// SDL: NON-OWNING
	SDL_Window* m_pWindow{};
	//

	// HARDWARE RESOURCES: OWNING
	ID3D11Device* m_pDevice{};
	ID3D11DeviceContext* m_pDeviceContext{};

	IDXGISwapChain* m_pSwapChain{};

	ID3D11Resource* m_pRenderTargetBuffer{};
	ID3D11RenderTargetView* m_pRenderTargetView{};

	ID3D11Texture2D* m_pDepthStencilBuffer{};
	ID3D11DepthStencilView* m_pDepthStencilView{};
	//

	// HARDWARE RESOURCES: NON-OWNING
	//

	// DIRECTX
	void InitializeDirectX();
	//
};
} // namespace dae

#endif
