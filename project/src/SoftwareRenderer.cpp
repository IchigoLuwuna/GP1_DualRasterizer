// External includes
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <cassert>
#include <execution>
#include <SDL_keyboard.h>
#include <d3dx11effect.h>

// Project includes
#include "SoftwareRenderer.h"
#include "Error.h"
#include "Scene.h"

#define PARALLEL_PROJECT

using namespace dae;

SoftwareRenderer::SoftwareRenderer( SDL_Window* pWindow )
	: m_pWindow( pWindow )
{
	// Initialize
	SDL_GetWindowSize( pWindow, &m_Width, &m_Height );

	// Create Buffers
	m_pFrontBuffer = SDL_GetWindowSurface( pWindow );
	m_pBackBuffer = SDL_CreateRGBSurface( 0, m_Width, m_Height, 32, 0, 0, 0, 0 );
	m_pBackBufferPixels = reinterpret_cast<uint32_t*>( m_pBackBuffer->pixels );
	m_DepthBufferPixels = std::vector<float>( m_Width * m_Height );
	m_PixelAttributeBuffer = std::vector<std::pair<bool, VertexOut>>( m_Width * m_Height );
}

SoftwareRenderer::~SoftwareRenderer()
{
	SDL_FreeSurface( m_pBackBuffer );
}

void SoftwareRenderer::Update( const Timer& timer )
{
	const uint8_t* pKeyboardState{ SDL_GetKeyboardState( nullptr ) };

	if ( pKeyboardState[SDL_SCANCODE_F4] && !m_F4Held )
	{
		m_F4Held = true;
		m_ShowDepthBuffer = !m_ShowDepthBuffer;
	}
	if ( !pKeyboardState[SDL_SCANCODE_F4] )
	{
		m_F4Held = false;
	}

	if ( pKeyboardState[SDL_SCANCODE_F6] && !m_F6Held )
	{
		m_F6Held = true;
		m_UseNormalMap = !m_UseNormalMap;
	}
	if ( !pKeyboardState[SDL_SCANCODE_F6] )
	{
		m_F6Held = false;
	}

	if ( !m_F7Held && pKeyboardState[SDL_SCANCODE_F7] )
	{
		m_LightingMode = static_cast<LightingMode>( ( static_cast<int>( m_LightingMode ) + 1 ) %
													static_cast<int>( LightingMode::count ) );
		m_F7Held = true;
	}
	else if ( m_F7Held && !pKeyboardState[SDL_SCANCODE_F7] )
	{
		m_F7Held = false;
	}
}

void SoftwareRenderer::Render( const Scene* pScene )
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

void SoftwareRenderer::RasterizeMesh( const Mesh& mesh, const Scene* pScene, const Matrix& worldToCamera )
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

void SoftwareRenderer::Project( const std::vector<Vertex>& verticesIn,
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

bool SoftwareRenderer::IsInPixel( const TriangleOut& triangle, int px, int py, Vector3& baryCentricPosition ) noexcept
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

bool SoftwareRenderer::IsCullable( const TriangleOut& triangle ) noexcept
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

bool SoftwareRenderer::SaveBufferToImage() const
{
	return SDL_SaveBMP( m_pBackBuffer, "Rasterizer_ColorBuffer.bmp" );
}
