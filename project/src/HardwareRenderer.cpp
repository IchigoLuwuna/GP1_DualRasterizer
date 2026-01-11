// External includes
#include "SDL.h"
#include "SDL_surface.h"

// Standard includes
#include <cassert>
#include <iostream>
#include <SDL_syswm.h>
#include <d3dx11effect.h>

// Project includes
#include "HardwareRenderer.h"
#include "Error.h"
#include "Mesh.h"
#include "Timer.h"

using namespace dae;

Renderer::Renderer( SDL_Window* pWindow )
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

	// Software: Create Buffers
	m_pFrontBuffer = SDL_GetWindowSurface( pWindow );
	m_pBackBuffer = SDL_CreateRGBSurface( 0, m_Width, m_Height, 32, 0, 0, 0, 0 );
	m_pBackBufferPixels = reinterpret_cast<uint32_t*>( m_pBackBuffer->pixels );
	m_DepthBufferPixels = std::vector<float>( m_Width * m_Height );
	m_PixelAttributeBuffer = std::vector<std::pair<bool, VertexOut>>( m_Width * m_Height );
	//
}

Renderer::~Renderer() noexcept
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

void Renderer::Update( const Timer& timer )
{
}

void Renderer::Render( Scene* pScene )
{
	if ( m_UseHardware )
	{
		RenderHW( pScene );
	}
	else
	{
		RenderSW( pScene );
	}
}

void Renderer::RenderHW( Scene* pScene )
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

void Renderer::RenderSW( Scene* pScene )
{
	//@START
	// Lock BackBuffer
	SDL_LockSurface( m_pBackBuffer );

	// Flush buffers
	for ( int px{}; px < m_Width; ++px )
	{
		for ( int py{}; py < m_Height; ++py )
		{
			const int lightGrey{ 99 };
			m_pBackBufferPixels[px + ( py * m_Width )] =
				SDL_MapRGB( m_pBackBuffer->format, lightGrey, lightGrey, lightGrey );
		}
	}
	for ( auto& depthPixel : m_DepthBufferPixels )
	{
		depthPixel = std::numeric_limits<float>::max();
	}

	// Get world to camera
	Matrix worldToCamera{ pScene->GetCamera().GetViewMatrix() };

	// For every mesh
	const auto& meshes{ pScene->GetMeshes() };
	for ( const auto& mesh : meshes )
	{
		RasterizeMesh( mesh, pScene, worldToCamera );
	}

	//@END
	// Update SDL Surface
	SDL_UnlockSurface( m_pBackBuffer );
	SDL_BlitSurface( m_pBackBuffer, 0, m_pFrontBuffer, 0 );
	SDL_UpdateWindowSurface( m_pWindow );
}

void Renderer::RasterizeMesh( const Mesh& mesh, const Scene* pScene, const Matrix& worldToCamera )
{
	const Camera& camera{ pScene->GetCamera() };

	// Flush pixel attribute buffer
	for ( auto& pixel : m_PixelAttributeBuffer )
	{
		pixel = {};
	}

	// PROJECTION
	Project( mesh.GetVertices(), m_VertexOutBuffer, camera, mesh.GetWorld(), worldToCamera );

	// For every triangle in mesh
	for ( size_t index{}; index < mesh.GetIndices().size(); )
	{
		auto goToNextTriangleIndex{ [&]() {
			// Increment differently based on topology
			switch ( mesh.GetTopology() )
			{
				// changed after original handin to comply with DX11 topology type
			case D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST:
				index += 3;
				break;

			case D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP:
				++index;
				break;

			default:
				throw error::mesh::WrongTopology();
				break;
			}
		} };

		// Stop if at the end of strip
		if ( index + 2 >= mesh.GetIndices().size() )
		{
			break;
		}

		// Construct triangle
		TriangleOut projectedTriangle{};
		switch ( mesh.GetTopology() )
		{
		case D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST:
			projectedTriangle = TriangleOut{ m_VertexOutBuffer[mesh.GetIndices()[index + 0]],
											 m_VertexOutBuffer[mesh.GetIndices()[index + 1]],
											 m_VertexOutBuffer[mesh.GetIndices()[index + 2]] };
			break;
		case D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP:
			// Check if mesh is correct size to be a strip
			assert( mesh.GetIndexCount() > 6 && "Mesh has too few indices to be a strip" );
			// Fix orientation for odd triangles
			if ( index & 1 )
			{
				projectedTriangle = TriangleOut{ m_VertexOutBuffer[mesh.GetIndices()[index + 0]],
												 m_VertexOutBuffer[mesh.GetIndices()[index + 2]],
												 m_VertexOutBuffer[mesh.GetIndices()[index + 1]] };
			}
			else
			{
				projectedTriangle = TriangleOut{ m_VertexOutBuffer[mesh.GetIndices()[index + 0]],
												 m_VertexOutBuffer[mesh.GetIndices()[index + 1]],
												 m_VertexOutBuffer[mesh.GetIndices()[index + 2]] };
			}
			break;

		default:
			throw error::mesh::WrongTopology();
			break;
		}

		if ( IsCullable( projectedTriangle ) )
		{
			goToNextTriangleIndex();
			continue;
		}

		const Rectangle projectedTriangleBounds{ projectedTriangle.GetBounds() };

		auto processPixel{ [&]( int px, int py ) {
			Vector3 baryCentricPosition{};
			if ( !IsInPixel( projectedTriangle, px, py, baryCentricPosition ) )
			{
				return;
			}

			const float interpolatedDepth{ 1.f /
										   ( ( 1.f / projectedTriangle.v0.position.z ) * baryCentricPosition.x +
											 ( 1.f / projectedTriangle.v1.position.z ) * baryCentricPosition.y +
											 ( 1.f / projectedTriangle.v2.position.z ) * baryCentricPosition.z ) };

			const int bufferIndex{ px + ( py * m_Width ) };

			// Check Depth Buffer
			if ( interpolatedDepth > m_DepthBufferPixels[bufferIndex] )
			{
				return;
			}
			m_DepthBufferPixels[bufferIndex] = interpolatedDepth;

			const float viewSpaceDepthInterpolated{
				1.f / ( ( 1.f / projectedTriangle.v0.position.w ) * baryCentricPosition.x +
						( 1.f / projectedTriangle.v1.position.w ) * baryCentricPosition.y +
						( 1.f / projectedTriangle.v2.position.w ) * baryCentricPosition.z )
			};

			Vector4 interpolatedPosition{};
			Vector3 interpolatedWorldPosition{};
			Vector3 interpolatedNormal{};
			Vector3 interpolatedTangent{};

			// Interpolate
			interpolatedPosition.x = projectedTriangle.v0.position.x * baryCentricPosition.x +
									 projectedTriangle.v1.position.x * baryCentricPosition.y +
									 projectedTriangle.v2.position.x * baryCentricPosition.z;
			interpolatedPosition.y = projectedTriangle.v0.position.y * baryCentricPosition.x +
									 projectedTriangle.v1.position.y * baryCentricPosition.y +
									 projectedTriangle.v2.position.y * baryCentricPosition.z;
			interpolatedPosition.w = projectedTriangle.v0.position.z * baryCentricPosition.x +
									 projectedTriangle.v1.position.z * baryCentricPosition.y +
									 projectedTriangle.v2.position.z * baryCentricPosition.z;
			interpolatedPosition.z = interpolatedDepth;

			interpolatedWorldPosition.x = projectedTriangle.v0.worldPosition.x * baryCentricPosition.x +
										  projectedTriangle.v1.worldPosition.x * baryCentricPosition.y +
										  projectedTriangle.v2.worldPosition.x * baryCentricPosition.z;
			interpolatedWorldPosition.y = projectedTriangle.v0.worldPosition.y * baryCentricPosition.x +
										  projectedTriangle.v1.worldPosition.y * baryCentricPosition.y +
										  projectedTriangle.v2.worldPosition.y * baryCentricPosition.z;
			interpolatedWorldPosition.z = projectedTriangle.v0.worldPosition.z * baryCentricPosition.x +
										  projectedTriangle.v1.worldPosition.z * baryCentricPosition.y +
										  projectedTriangle.v2.worldPosition.z * baryCentricPosition.z;

			const ColorRGB interpolatedColor{
				( projectedTriangle.v0.color / projectedTriangle.v0.position.w * baryCentricPosition.x +
				  projectedTriangle.v1.color / projectedTriangle.v1.position.w * baryCentricPosition.y +
				  projectedTriangle.v2.color / projectedTriangle.v2.position.w * baryCentricPosition.z ) *
				viewSpaceDepthInterpolated
			};

			const Vector2 interpolatedUV{
				( projectedTriangle.v0.uv / projectedTriangle.v0.position.w * baryCentricPosition.x +
				  projectedTriangle.v1.uv / projectedTriangle.v1.position.w * baryCentricPosition.y +
				  projectedTriangle.v2.uv / projectedTriangle.v2.position.w * baryCentricPosition.z ) *
				viewSpaceDepthInterpolated
			};

			interpolatedNormal.x = projectedTriangle.v0.normal.x * baryCentricPosition.x +
								   projectedTriangle.v1.normal.x * baryCentricPosition.y +
								   projectedTriangle.v2.normal.x * baryCentricPosition.z;
			interpolatedNormal.y = projectedTriangle.v0.normal.y * baryCentricPosition.x +
								   projectedTriangle.v1.normal.y * baryCentricPosition.y +
								   projectedTriangle.v2.normal.y * baryCentricPosition.z;
			interpolatedNormal.z = projectedTriangle.v0.normal.z * baryCentricPosition.x +
								   projectedTriangle.v1.normal.z * baryCentricPosition.y +
								   projectedTriangle.v2.normal.z * baryCentricPosition.z;
			interpolatedNormal.Normalize();

			interpolatedTangent.x = projectedTriangle.v0.tangent.x * baryCentricPosition.x +
									projectedTriangle.v1.tangent.x * baryCentricPosition.y +
									projectedTriangle.v2.tangent.x * baryCentricPosition.z;
			interpolatedTangent.y = projectedTriangle.v0.tangent.y * baryCentricPosition.x +
									projectedTriangle.v1.tangent.y * baryCentricPosition.y +
									projectedTriangle.v2.tangent.y * baryCentricPosition.z;
			interpolatedTangent.z = projectedTriangle.v0.tangent.z * baryCentricPosition.x +
									projectedTriangle.v1.tangent.z * baryCentricPosition.y +
									projectedTriangle.v2.tangent.z * baryCentricPosition.z;
			interpolatedTangent.Normalize();

			VertexOut interpolatedVertex{ interpolatedPosition, interpolatedWorldPosition, interpolatedColor,
										  interpolatedUV,		interpolatedNormal,		   interpolatedTangent };

			m_PixelAttributeBuffer[bufferIndex].first = true;
			m_PixelAttributeBuffer[bufferIndex].second = interpolatedVertex;
		} };

		const int pixelBoundsLeft{ static_cast<int>( std::floor( projectedTriangleBounds.left ) ) };
		const int pixelBoundsRight{ static_cast<int>( std::ceil( projectedTriangleBounds.right ) ) };
		const int pixelBoundsTop{ static_cast<int>( std::floor( projectedTriangleBounds.top ) ) };
		const int pixelBoundsBottom{ static_cast<int>( std::ceil( projectedTriangleBounds.bottom ) ) };

		// RASTERIZATION
		for ( int px{ pixelBoundsLeft }; px < pixelBoundsRight; ++px )
		{
			for ( int py{ pixelBoundsTop }; py < pixelBoundsBottom; ++py )
			{
				processPixel( px, py );
			}
		}

		goToNextTriangleIndex();
	}

	for ( int px{}; px < m_Width; ++px )
	{
		for ( int py{}; py < m_Height; ++py )
		{
			const int bufferIndex{ px + ( py * m_Width ) };

			if ( !m_PixelAttributeBuffer[bufferIndex].first )
			{
				continue;
			}
			if ( m_ShowDepthBuffer )
			{
				constexpr float depthMin{ 0.9985f };
				constexpr float depthMax{ 1.f };
				const float remappedDepth{ std::max(
					1.f - ( m_DepthBufferPixels[bufferIndex] - depthMin ) / ( depthMax - depthMin ), 0.f ) };
				ColorRGB finalColor{ remappedDepth, remappedDepth, remappedDepth };

				finalColor.MaxToOne();

				m_pBackBufferPixels[bufferIndex] = SDL_MapRGB( m_pBackBuffer->format,
															   static_cast<uint8_t>( finalColor.r * 255 ),
															   static_cast<uint8_t>( finalColor.g * 255 ),
															   static_cast<uint8_t>( finalColor.b * 255 ) );

				continue;
			}

			const ColorRGB finalColor{ GetPixelColor( m_PixelAttributeBuffer[bufferIndex].second,
													  mesh.GetDiffuseMap(),
													  mesh.GetNormalMap(),
													  mesh.GetSpecularMap(),
													  mesh.GetGlossMap(),
													  camera,
													  pScene->GetLightDirection(),
													  m_LightingMode,
													  m_UseNormalMap ) };

			m_pBackBufferPixels[bufferIndex] = SDL_MapRGB( m_pBackBuffer->format,
														   static_cast<uint8_t>( finalColor.r * 255 ),
														   static_cast<uint8_t>( finalColor.g * 255 ),
														   static_cast<uint8_t>( finalColor.b * 255 ) );
		}
	}
}

void Renderer::Project( const std::vector<Vertex>& verticesIn,
						std::vector<VertexOut>& verticesOut,
						const Camera& camera,
						const Matrix& modelToWorld,
						const Matrix& worldToCamera ) const noexcept
{
	verticesOut.clear();
	verticesOut.reserve( verticesIn.size() );

	std::vector<uint32_t> vertexIndices{};
	vertexIndices.reserve( verticesIn.size() );
	for ( size_t index{}; index < verticesIn.size(); ++index )
	{
		verticesOut.push_back( {} );
		vertexIndices.push_back( index );
	}

	auto projectVertex{ [&]( const uint32_t index ) {
		const Vertex& vertexIn{ verticesIn[index] };
		VertexOut vertexOut{};
		vertexOut.uv = vertexIn.uv;

		// World transform
		vertexOut.worldPosition = modelToWorld.TransformPoint( vertexIn.position );
		vertexOut.position = vertexOut.worldPosition.ToPoint4();
		vertexOut.normal = modelToWorld.TransformVector( vertexIn.normal ).Normalized();
		vertexOut.tangent = modelToWorld.TransformVector( vertexIn.tangent ).Normalized();

		// Camera transform
		vertexOut.position = worldToCamera.TransformPoint( vertexOut.position );
		// vertexOut.normal = worldToCamera.TransformVector( vertexOut.normal );

		// Projection matrix
		const float aspectRatio{ static_cast<float>( m_Width ) / m_Height };
		const float a{ camera.GetFar() / ( camera.GetFar() - camera.GetNear() ) }; // Depends on coordinate system
		const float b{ -( camera.GetFar() * camera.GetNear() ) / ( camera.GetFar() - camera.GetNear() ) };
		Matrix projectionMatrix{
			{ 1.f / ( aspectRatio * camera.GetFov() ), 0.f, 0.f, 0.f },
			{ 0.f, 1.f / camera.GetFov(), 0.f, 0.f },
			{ 0.f, 0.f, a, 1.f },
			{ 0.f, 0.f, b, 0.f },
		};
		vertexOut.position = projectionMatrix.TransformPoint( vertexOut.position );

		// Perspective divide
		vertexOut.position.x /= vertexOut.position.w;
		vertexOut.position.y /= vertexOut.position.w;
		vertexOut.position.z /= vertexOut.position.w;

		// To screenspace
		vertexOut.position.x = ( 1.f + vertexOut.position.x ) * 0.5f * m_Width;
		vertexOut.position.y = ( 1.f - vertexOut.position.y ) * 0.5f * m_Height;

		verticesOut[index] = vertexOut;
	} };

#ifdef PARALLEL_PROJECT
	std::for_each( std::execution::par, vertexIndices.begin(), vertexIndices.end(), projectVertex );
#endif
#ifndef PARALLEL_PROJECT
	for ( const auto& vertexIndex : vertexIndices )
	{
		projectVertex( vertexIndex );
	}
#endif
}

bool Renderer::IsInPixel( const TriangleOut& triangle, int px, int py, Vector3& baryCentricPosition ) noexcept
{
	const Vector2 screenSpace{ px + 0.5f, py + 0.5f };

	const std::array<Vector2, 3> triangle2d{ triangle.v0.position.GetXY(),
											 triangle.v1.position.GetXY(),
											 triangle.v2.position.GetXY() };

	std::array<float, 3> baryBuffer{};
	const float parallelogramArea{ Vector2::Cross( ( triangle2d[1] - triangle2d[0] ),
												   ( triangle2d[2] - triangle2d[0] ) ) };

	// Prevent floating point precision error-based crashes
	// This area being 0 or lower could pose issues later down the line
	if ( parallelogramArea <= 0.f )
	{
		return false;
	}

	for ( int offset{}; offset < 3; ++offset )
	{
		int nextOffset{ ( offset + 1 ) % 3 };
		const Vector2 vertex{ triangle2d[offset] };
		const Vector2 nextVertex{ triangle2d[nextOffset] };
		const Vector2 edgeVector{ nextVertex - vertex };
		const Vector2 vertexToPixel{ screenSpace - vertex };

		const float signedArea{ Vector2::Cross( edgeVector, vertexToPixel ) };

		if ( signedArea < 0.f )
		{
			return false;
		}
		else
		{
			const int baryIndex{ ( offset + 2 ) % 3 };
			baryBuffer[baryIndex] = signedArea / parallelogramArea;
			continue;
		}
	}

	baryCentricPosition = { baryBuffer[0], baryBuffer[1], baryBuffer[2] };

	return true;
}

bool Renderer::IsCullable( const TriangleOut& triangle ) noexcept
{
	// Backface Culling
	if ( triangle.normal.z > 0.f ) // positive Z is forward -> away from the screen
	{
		return true;
	}

	// Frustum Culling
	if ( triangle.v0.position.z > 1.f || triangle.v0.position.z < 0.f )
	{
		return true;
	}
	if ( triangle.v1.position.z > 1.f || triangle.v1.position.z < 0.f )
	{
		return true;
	}
	if ( triangle.v2.position.z > 1.f || triangle.v2.position.z < 0.f )
	{
		return true;
	}

	// ScreenSpace Culling
	if ( triangle.v0.position.x < 0.f || triangle.v0.position.x > m_Width )
	{
		return true;
	}
	if ( triangle.v1.position.x < 0.f || triangle.v1.position.x > m_Width )
	{
		return true;
	}
	if ( triangle.v2.position.x < 0.f || triangle.v2.position.x > m_Width )
	{
		return true;
	}
	if ( triangle.v0.position.y < 0.f || triangle.v0.position.y > m_Height )
	{
		return true;
	}
	if ( triangle.v1.position.y < 0.f || triangle.v1.position.y > m_Height )
	{
		return true;
	}
	if ( triangle.v2.position.y < 0.f || triangle.v2.position.y > m_Height )
	{
		return true;
	}

	return false;
}

void Renderer::InitScene( Scene* pScene )
{
	pScene->Initialize( m_pDevice, ( static_cast<float>( m_Width ) / m_Height ) );
}

void Renderer::InitializeDirectX()
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
