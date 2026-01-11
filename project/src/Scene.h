#ifndef SCENE_H
#define SCENE_H
#include "Camera.h"
#include "Mesh.h"

namespace dae
{
class Scene
{
public:
	Scene() = default;

	virtual void Update( Timer* pTimer );
	virtual void Draw( ID3D11DeviceContext* pDeviceContext );

	virtual void Initialize( ID3D11Device* pDevice, float aspectRatio ) = 0;

	// Software
	const Camera& GetCamera() const;
	const std::vector<Mesh>& GetMeshes() const;
	Vector3 GetLightDirection() const;
	//

protected:
	Camera m_Camera{};
	std::vector<Mesh> m_Meshes{};
	std::vector<TransparentMesh> m_TransparentMeshes{};
	Vector3 m_LightDir{};

	// TODO:Make this a bitmask
	bool m_F2Held{};
};

class TestScene : public Scene
{
	virtual void Initialize( ID3D11Device* pDevice, float aspectRatio ) override;
};

class VehicleScene : public Scene
{
public:
	virtual void Update( Timer* pTimer ) override;

	virtual void Initialize( ID3D11Device* pDevice, float aspectRatio ) override;
};
} // namespace dae

#endif
