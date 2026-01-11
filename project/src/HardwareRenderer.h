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
#include "Shading.h"

namespace dae
{
class Renderer final
{
public:
	Renderer( SDL_Window* pWindow );
	~Renderer() noexcept;

	Renderer( const Renderer& ) = delete;
	Renderer( Renderer&& ) noexcept = delete;
	Renderer& operator=( const Renderer& ) = delete;
	Renderer& operator=( Renderer&& ) noexcept = delete;

	void Update( const Timer& timer );
	void Render( Scene* pScene );
	void RenderHW( Scene* pScene );
	void RenderSW( Scene* pScene );

	void InitScene( Scene* pScene );

private:
	int m_Width{};
	int m_Height{};

	bool m_IsInitialized{ false };
	bool m_UseHardware{ false };

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

	// SOFTWARE RENDERER
	SDL_Surface* m_pFrontBuffer{ nullptr };
	SDL_Surface* m_pBackBuffer{ nullptr };
	uint32_t* m_pBackBufferPixels{};

	std::vector<float> m_DepthBufferPixels{};
	std::vector<std::pair<bool, VertexOut>> m_PixelAttributeBuffer{};

	std::vector<VertexOut> m_VertexOutBuffer{};

	LightingMode m_LightingMode{ LightingMode::combined };

	bool m_ShowDepthBuffer{};
	bool m_UseNormalMap{ true };

	bool m_F4Held{};
	bool m_F6Held{};
	bool m_F7Held{};

	void Project( const std::vector<Vertex>& verticesIn,
				  std::vector<VertexOut>& verticesOut,
				  const Camera& camera,
				  const Matrix& modelToWorld,
				  const Matrix& worldToCamera ) const noexcept;
	void RasterizeMesh( const Mesh& mesh, const Scene* pScene, const Matrix& worldToCamera );
	void ShadePixel( int px, int py, const VertexOut& attributes );

	bool IsInPixel( const TriangleOut& triangle, int px, int py, Vector3& baryCentricPosition ) noexcept;
	bool IsCullable( const TriangleOut& triangle ) noexcept;
	//
};
} // namespace dae

#endif
