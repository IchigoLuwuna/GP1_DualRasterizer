// External includes
#include "SDL.h"
#include "SDL_surface.h"

// Standard includes
#include <iostream>
#include <SDL_syswm.h>
#include <d3dx11effect.h>

// Project includes
#include "HardwareRenderer.h"
#include "Error.h"
#include "Mesh.h"
#include "Timer.h"

using namespace dae;

HardwareRenderer::HardwareRenderer( SDL_Window* pWindow )
	: m_pWindow( pWindow )
{
	// Initialize Window
	SDL_GetWindowSize( pWindow, &m_Width, &m_Height );

	//
	const bool failed{ error::utils::HandleThrowingFunction( [&]() { InitializeDirectX(); } ) };
	if ( failed )
	{
		std::cout << "DirectX initialisation failed\n";
	}
	else
	{
		m_IsInitialized = true;
		std::cout << "DirectX is initialized and ready\n";
	}
}

HardwareRenderer::~HardwareRenderer() noexcept
{
	if ( m_pRenderTargetView )
	{
		m_pRenderTargetView->Release();
	}

	if ( m_pRenderTargetBuffer )
	{
		m_pRenderTargetBuffer->Release();
	}

	if ( m_pDepthStencilView )
	{
		m_pDepthStencilView->Release();
	}

	if ( m_pDepthStencilBuffer )
	{
		m_pDepthStencilBuffer->Release();
	}

	if ( m_pSwapChain )
	{
		m_pSwapChain->Release();
	}

	if ( m_pDeviceContext )
	{
		m_pDeviceContext->ClearState();
		m_pDeviceContext->Flush();
		m_pDeviceContext->Release();
	}

	if ( m_pDevice )
	{
		m_pDevice->Release();
	}
}

void HardwareRenderer::Update( const Timer& timer )
{
}

void HardwareRenderer::Render( Scene* pScene )
{
	if ( !m_IsInitialized )
	{
		return;
	}

	// 1. Clear RTV & DSV
	const float cornflowerBlue[4]{ 0.39f, 0.59f, 0.93f, 1.f };
	m_pDeviceContext->ClearRenderTargetView( m_pRenderTargetView, cornflowerBlue );
	m_pDeviceContext->ClearDepthStencilView( m_pDepthStencilView, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.f, 0 );

	// 2. Draw
	const bool failed{ error::utils::HandleThrowingFunction( [&]() { pScene->Draw( m_pDeviceContext ); } ) };
	if ( failed )
	{
		m_IsInitialized = false;
		std::cout << "Renderer has encountered an error\nShutting down!\n";
	}

	// 3. Present backbuffer
	m_pSwapChain->Present( 0, 0 );
}

void HardwareRenderer::InitScene( Scene* pScene )
{
	pScene->Initialize( m_pDevice, ( static_cast<float>( m_Width ) / m_Height ) );
}

void HardwareRenderer::InitializeDirectX()
{
	// 1. Create device context
	const D3D_FEATURE_LEVEL featureLevel{ D3D_FEATURE_LEVEL_11_1 };
	uint32_t createDeviceFlags{};

#if defined( DEBUG ) || defined( _DEBUG )
	createDeviceFlags |= D3D11_CREATE_DEVICE_DEBUG;
#endif
	HRESULT result{ D3D11CreateDevice( nullptr,
									   D3D_DRIVER_TYPE_HARDWARE,
									   0,
									   createDeviceFlags,
									   &featureLevel,
									   1,
									   D3D11_SDK_VERSION,
									   &m_pDevice,
									   nullptr,
									   &m_pDeviceContext ) };
	if ( FAILED( result ) )
	{
		throw error::dx11::DeviceCreateFail();
	}

	// Create DXGI Factory
	IDXGIFactory1* pDxgiFactory{};
	result = CreateDXGIFactory1( __uuidof( IDXGIFactory1 ), reinterpret_cast<void**>( &pDxgiFactory ) );
	if ( FAILED( result ) )
	{
		pDxgiFactory->Release();
		throw error::dx11::DXGIFactoryCreateFail();
	}

	// 2. Create swapchain
	DXGI_SWAP_CHAIN_DESC swapChainDesc{};
	swapChainDesc.BufferDesc.Width = m_Width;
	swapChainDesc.BufferDesc.Height = m_Height;
	swapChainDesc.BufferDesc.RefreshRate.Numerator = 1;
	swapChainDesc.BufferDesc.RefreshRate.Denominator = 60;
	swapChainDesc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	swapChainDesc.BufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
	swapChainDesc.BufferDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;
	swapChainDesc.SampleDesc.Count = 1;
	swapChainDesc.SampleDesc.Quality = 0;
	swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	swapChainDesc.BufferCount = 1;
	swapChainDesc.Windowed = 1;
	swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
	swapChainDesc.Flags = 0;

	// Get the handle (HWND) from the SDL backbuffer
	SDL_SysWMinfo sysWMInfo{};
	SDL_GetVersion( &sysWMInfo.version );
	SDL_GetWindowWMInfo( m_pWindow, &sysWMInfo );
	swapChainDesc.OutputWindow = sysWMInfo.info.win.window;
	result = pDxgiFactory->CreateSwapChain( m_pDevice, &swapChainDesc, &m_pSwapChain );
	if ( FAILED( result ) )
	{
		pDxgiFactory->Release();
		throw error::dx11::SwapChainCreateFail();
	}

	// 3. Create DepthStencil (DS) & DepthStencilView (DSV)
	// Resource
	D3D11_TEXTURE2D_DESC depthStencilDesc{};
	depthStencilDesc.Width = m_Width;
	depthStencilDesc.Height = m_Height;
	depthStencilDesc.MipLevels = 1;
	depthStencilDesc.ArraySize = 1;
	depthStencilDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
	depthStencilDesc.SampleDesc.Count = 1;
	depthStencilDesc.SampleDesc.Quality = 0;
	depthStencilDesc.Usage = D3D11_USAGE_DEFAULT;
	depthStencilDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL;
	depthStencilDesc.CPUAccessFlags = 0;
	depthStencilDesc.MiscFlags = 0;

	// View
	D3D11_DEPTH_STENCIL_VIEW_DESC depthStencilViewDesc{};
	depthStencilViewDesc.Format = depthStencilDesc.Format;
	depthStencilViewDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
	depthStencilViewDesc.Texture2D.MipSlice = 0;

	result = m_pDevice->CreateTexture2D( &depthStencilDesc, nullptr, &m_pDepthStencilBuffer );
	if ( FAILED( result ) )
	{
		pDxgiFactory->Release();
		throw error::dx11::DepthStencilCreateFail();
	}

	result = m_pDevice->CreateDepthStencilView( m_pDepthStencilBuffer, &depthStencilViewDesc, &m_pDepthStencilView );
	if ( FAILED( result ) )
	{
		pDxgiFactory->Release();
		throw error::dx11::DepthStencilViewCreateFail();
	}

	// 4. Create RenderTarget (RT) & RenderTargetView (RTV)
	// Resource
	result =
		m_pSwapChain->GetBuffer( 0, __uuidof( ID3D11Texture2D ), reinterpret_cast<void**>( &m_pRenderTargetBuffer ) );
	if ( FAILED( result ) )
	{
		pDxgiFactory->Release();
		throw error::dx11::GetRenderTargetBufferFail();
	}

	// View
	result = m_pDevice->CreateRenderTargetView( m_pRenderTargetBuffer, nullptr, &m_pRenderTargetView );
	if ( FAILED( result ) )
	{
		pDxgiFactory->Release();
		throw error::dx11::RenderTargetViewCreateFail();
	}

	// 5. Bind RTV & DSV to Output Merger Stage
	m_pDeviceContext->OMSetRenderTargets( 1, &m_pRenderTargetView, m_pDepthStencilView );

	// 6. Set viewport
	D3D11_VIEWPORT viewport{};
	viewport.Width = static_cast<float>( m_Width );
	viewport.Height = static_cast<float>( m_Height );
	viewport.TopLeftX = 0.f;
	viewport.TopLeftY = 0.f;
	viewport.MinDepth = 0.f;
	viewport.MaxDepth = 1.f;
	m_pDeviceContext->RSSetViewports( 1, &viewport );

	pDxgiFactory->Release();
}
