#include <SDL_keyboard.h>
#include <d3dx11effect.h>
#include "Scene.h"
#include "Error.h"
#include "Utils.h"

namespace dae
{
void Scene::Update( Timer* pTimer )
{
	// Update Camera
	m_Camera.Update( pTimer );

	for ( auto& mesh : m_Meshes )
	{
		mesh.SetWorldViewProjection( m_Camera.GetPosition(), m_Camera.GetViewMatrix(), m_Camera.GetProjectionMatrix() );
	}

	for ( auto& transparentMesh : m_TransparentMeshes )
	{
		transparentMesh.SetWorldViewProjection( m_Camera.GetViewMatrix(), m_Camera.GetProjectionMatrix() );
	}
	//

	// Handle input
	const Uint8* pKeyboardState{ SDL_GetKeyboardState( nullptr ) };
	if ( pKeyboardState[SDL_SCANCODE_F2] && !m_F2Held )
	{
		m_F2Held = true;
		for ( auto& mesh : m_Meshes )
		{
			mesh.CycleFilteringMode();
		}

		for ( auto& transparentMesh : m_TransparentMeshes )
		{
			transparentMesh.CycleFilteringMode();
		}
	}
	if ( !pKeyboardState[SDL_SCANCODE_F2] && m_F2Held )
	{
		m_F2Held = false;
	}
	//
}

void Scene::Draw( ID3D11DeviceContext* pDeviceContext )
{
	if ( m_Meshes.empty() && m_TransparentMeshes.empty() )
	{
		throw error::scene::SceneIsEmpty();
	}

	// Draw normal meshes
	for ( auto& mesh : m_Meshes )
	{
		mesh.Draw( pDeviceContext );
	}

	// Draw transparent meshes
	for ( auto& transparentMesh : m_TransparentMeshes )
	{
		transparentMesh.Draw( pDeviceContext );
	}
}

const Camera& Scene::GetCamera() const
{
	return m_Camera;
}

const std::vector<Mesh>& Scene::GetMeshes() const
{
	return m_Meshes;
}

Vector3 Scene::GetLightDirection() const
{
	return { 0.577, -0.577, 0.577 };
}

void VehicleScene::Update( Timer* pTimer )
{
	const Matrix rotation{ Matrix::CreateRotationY( pTimer->GetElapsed() * 0.25f * PI ) };

	m_Meshes[0].ApplyMatrix( rotation );
	m_TransparentMeshes[0].ApplyMatrix( rotation );

	Scene::Update( pTimer );
}

void VehicleScene::Initialize( ID3D11Device* pDevice, float aspectRatio )
{
	m_Camera = Camera{ { 0.f, 0.f, 0.f }, 45.f, aspectRatio, 0.1f, 100.f };

	// Uncomment if on C++26 -> std::sqrt is made constexpr
	// constexpr float xyzNormalized{1.f / std::sqrt(3.f)};
	// m_LightDir = { xyzNormalized, -xyzNormalized, xyzNormalized };

	// Comment if on C++26 -> non-magic number solution above
	m_LightDir = { 0.577f, -0.577f, 0.577f };

	std::vector<Vertex> vertices{};
	std::vector<uint32_t> indices{};

	Utils::ParseOBJ( "./resources/vehicle.obj", vertices, indices );
	const D3D11_PRIMITIVE_TOPOLOGY topology{ D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST };
	const std::wstring effectPath{ L"./resources/Opaque.fx" };
	const std::string diffuseMapPath{ "./resources/vehicle_diffuse.png" };
	const std::string normalMapPath{ "./resources/vehicle_normal.png" };
	const std::string specularMapPath{ "./resources/vehicle_specular.png" };
	const std::string glossMapPath{ "./resources/vehicle_gloss.png" };

	Mesh vehicle{
		pDevice, vertices, indices, topology, effectPath, diffuseMapPath, normalMapPath, specularMapPath, glossMapPath,
	};

	vehicle.ApplyMatrix( Matrix::CreateTranslation( 0.f, 0.f, 50.f ) );

	m_Meshes.push_back( std::move( vehicle ) );

	vertices.clear();
	indices.clear();

	Utils::ParseOBJ( "./resources/fireFX.obj", vertices, indices );
	const std::wstring partialCoverageEffectPath{ L"./resources/PartialCoverage.fx" };
	const std::string fireDiffuseMapPath{ "./resources/fireFX_diffuse.png" };

	TransparentMesh fire{
		pDevice, vertices, indices, topology, partialCoverageEffectPath, fireDiffuseMapPath,
	};

	fire.ApplyMatrix( Matrix::CreateTranslation( 0.f, 0.f, 50.f ) );

	m_TransparentMeshes.push_back( std::move( fire ) );
}
} // namespace dae
