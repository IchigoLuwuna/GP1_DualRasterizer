#ifndef MESH_H
#define MESH_H
#include <vector>
#include "Effect.h"

namespace dae
{
class Mesh final
{
public:
	Mesh() = default;
	Mesh( ID3D11Device* pDevice,
		  const std::vector<Vertex>& vertices,
		  const std::vector<UINT>& indices,
		  D3D11_PRIMITIVE_TOPOLOGY topology,
		  const std::wstring& effectPath,
		  const std::string& diffuseMapPath,
		  const std::string& normalMapPath,
		  const std::string& specularMapPath,
		  const std::string& glossMapPath );
	Mesh( const Mesh& ) = delete;
	Mesh( Mesh&& rhs );

	Mesh& operator=( const Mesh& ) = delete;
	Mesh& operator=( Mesh&& rhs );

	~Mesh() noexcept;

	// Methods
	void Draw( ID3D11DeviceContext* pDeviceContext ) const;
	void CycleFilteringMode();
	void ApplyMatrix( const Matrix& action );

	// Setters
	void SetWorldViewProjection( const Vector3& o, const Matrix& v, const Matrix& p );
	void SetWorld( const Matrix& w );

	// Getters
	ID3D11Buffer* GetVertexBufferPtr() const;
	ID3D11Buffer* GetIndexBufferPtr() const;
	Effect* GetEffectPtr();
	uint32_t GetVertexCount() const;
	uint32_t GetIndexCount() const;

	// For software
	const std::vector<Vertex>& GetVertices() const;
	const std::vector<UINT>& GetIndices() const;
	const Matrix& GetWorld() const;
	D3D11_PRIMITIVE_TOPOLOGY GetTopology() const;

	const Texture& GetDiffuseMap() const;
	const Texture& GetNormalMap() const;
	const Texture& GetSpecularMap() const;
	const Texture& GetGlossMap() const;
	//

private:
	// SOFTWARE RESOURCES
	uint32_t m_VertexCount{};
	uint32_t m_IndexCount{};
	D3D11_PRIMITIVE_TOPOLOGY m_Topology{};
	Matrix m_WorldMatrix{ Matrix::CreateIdentity() };
	//

	// HARDWARE RESOURCES: OWNING
	ID3D11Buffer* m_pVertexBuffer{};
	ID3D11Buffer* m_pIndexBuffer{};
	Effect m_Effect{};
	Texture m_DiffuseMap{};
	Texture m_NormalMap{};
	Texture m_SpecularMap{};
	Texture m_GlossMap{};
	//

	// For software rendering
	std::vector<Vertex> m_Vertices{};
	std::vector<UINT> m_Indices{};
	//
};

class TransparentMesh final // no inheritance because transparent meshes have to be handled differently
{
public:
	TransparentMesh() = default;
	TransparentMesh( ID3D11Device* pDevice,
					 const std::vector<Vertex>& vertices,
					 const std::vector<UINT>& indices,
					 D3D11_PRIMITIVE_TOPOLOGY topology,
					 const std::wstring& effectPath,
					 const std::string& diffuseMapPath );

	TransparentMesh( const TransparentMesh& ) = delete;
	TransparentMesh( TransparentMesh&& rhs );

	TransparentMesh& operator=( const TransparentMesh& ) = delete;
	TransparentMesh& operator=( TransparentMesh&& rhs );

	~TransparentMesh() noexcept;

	// Methods
	void Draw( ID3D11DeviceContext* pDeviceContext ) const;
	void CycleFilteringMode();
	void ApplyMatrix( const Matrix& action );

	// Setters
	void SetWorldViewProjection( const Matrix& v, const Matrix& p );
	void SetWorld( const Matrix& w );

	// Getters
	ID3D11Buffer* GetVertexBufferPtr() const;
	ID3D11Buffer* GetIndexBufferPtr() const;
	TransparentEffect* GetEffectPtr();
	uint32_t GetVertexCount() const;
	uint32_t GetIndexCount() const;

private:
	// SOFTWARE RESOURCES
	uint32_t m_VertexCount{};
	uint32_t m_IndexCount{};
	D3D11_PRIMITIVE_TOPOLOGY m_Topology{};
	Matrix m_WorldMatrix{ Matrix::CreateIdentity() };

	// HARDWARE RESOURCES: OWNING
	ID3D11Buffer* m_pVertexBuffer{};
	ID3D11Buffer* m_pIndexBuffer{};
	TransparentEffect m_Effect{};
	Texture m_DiffuseMap{};
	//
};
}; // namespace dae
#endif
