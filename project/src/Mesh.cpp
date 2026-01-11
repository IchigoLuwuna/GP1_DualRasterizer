#include "Mesh.h"
#include "Error.h"

namespace dae
{
Mesh::Mesh( ID3D11Device* pDevice,
			const std::vector<Vertex>& vertices,
			const std::vector<UINT>& indices,
			D3D11_PRIMITIVE_TOPOLOGY topology,
			const std::wstring& effectPath,
			const std::string& diffuseMapPath,
			const std::string& normalMapPath,
			const std::string& specularMapPath,
			const std::string& glossMapPath )
	: m_Topology( topology )
	, m_Effect( pDevice, effectPath )
	, m_DiffuseMap( pDevice, diffuseMapPath )
	, m_NormalMap( pDevice, normalMapPath )
	, m_SpecularMap( pDevice, specularMapPath )
	, m_GlossMap( pDevice, glossMapPath )
	, m_Vertices( vertices )
	, m_Indices( indices )
{
	if ( vertices.size() == 0 )
	{
		throw error::mesh::BufferIsEmpty();
	}

	if ( indices.size() == 0 )
	{
		throw error::mesh::BufferIsEmpty();
	}
	m_VertexCount = vertices.size();
	m_IndexCount = indices.size();

	// Create Vertex Buffer
	D3D11_BUFFER_DESC vertexBufferDesc{};
	vertexBufferDesc.Usage = D3D11_USAGE_IMMUTABLE;
	vertexBufferDesc.ByteWidth = sizeof( Vertex ) * m_VertexCount;
	vertexBufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;

	D3D11_SUBRESOURCE_DATA vertexData{};
	vertexData.pSysMem = vertices.data();

	HRESULT result{ pDevice->CreateBuffer( &vertexBufferDesc, &vertexData, &m_pVertexBuffer ) };
	if ( FAILED( result ) )
	{
		throw static_cast<int>( result );
	}
	//

	// Create Index Buffer
	D3D11_BUFFER_DESC indexBufferDesc{};
	indexBufferDesc.Usage = D3D11_USAGE_IMMUTABLE;
	indexBufferDesc.ByteWidth = sizeof( UINT ) * m_IndexCount;
	indexBufferDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;

	D3D11_SUBRESOURCE_DATA indexData{};
	indexData.pSysMem = indices.data();

	result = pDevice->CreateBuffer( &indexBufferDesc, &indexData, &m_pIndexBuffer );
	if ( FAILED( result ) )
	{
		throw static_cast<int>( result );
	}
	//

	// Pass texture view to effect
	m_Effect.SetDiffuseMap( m_DiffuseMap );
	m_Effect.SetNormalMap( m_NormalMap );
	m_Effect.SetSpecularMap( m_SpecularMap );
	m_Effect.SetGlossMap( m_GlossMap );
	//
}

Mesh::Mesh( Mesh&& rhs )
{
	if ( this == &rhs )
	{
		return;
	}

	m_VertexCount = rhs.m_VertexCount;
	m_IndexCount = rhs.m_IndexCount;
	m_Topology = rhs.m_Topology;
	m_WorldMatrix = rhs.m_WorldMatrix;

	m_pVertexBuffer = rhs.m_pVertexBuffer;
	rhs.m_pVertexBuffer = nullptr;

	m_pIndexBuffer = rhs.m_pIndexBuffer;
	rhs.m_pIndexBuffer = nullptr;

	m_Effect = std::move( rhs.m_Effect );
	m_DiffuseMap = std::move( rhs.m_DiffuseMap );
	m_NormalMap = std::move( rhs.m_NormalMap );
	m_SpecularMap = std::move( rhs.m_SpecularMap );
	m_GlossMap = std::move( rhs.m_GlossMap );

	m_Vertices = std::move( rhs.m_Vertices );
	m_Indices = std::move( rhs.m_Indices );
}

Mesh& Mesh::operator=( Mesh&& rhs )
{
	if ( this == &rhs )
	{
		return *this;
	}

	m_VertexCount = rhs.m_VertexCount;
	m_IndexCount = rhs.m_IndexCount;
	m_Topology = rhs.m_Topology;
	m_WorldMatrix = rhs.m_WorldMatrix;

	m_pVertexBuffer = rhs.m_pVertexBuffer;
	rhs.m_pVertexBuffer = nullptr;

	m_pIndexBuffer = rhs.m_pIndexBuffer;
	rhs.m_pIndexBuffer = nullptr;

	m_Effect = std::move( rhs.m_Effect );
	m_DiffuseMap = std::move( rhs.m_DiffuseMap );
	m_NormalMap = std::move( rhs.m_NormalMap );
	m_SpecularMap = std::move( rhs.m_SpecularMap );
	m_GlossMap = std::move( rhs.m_GlossMap );

	m_Vertices = std::move( rhs.m_Vertices );
	m_Indices = std::move( rhs.m_Indices );

	return *this;
}

Mesh::~Mesh() noexcept
{
	if ( m_pVertexBuffer )
	{
		m_pVertexBuffer->Release();
	}

	if ( m_pIndexBuffer )
	{
		m_pIndexBuffer->Release();
	}
}

void Mesh::Draw( ID3D11DeviceContext* pDeviceContext ) const
{
	// 1. Set primitive topology
	pDeviceContext->IASetPrimitiveTopology( m_Topology );

	// 2. Set input layout
	pDeviceContext->IASetInputLayout( m_Effect.GetInputLayoutPtr() );

	// 3. Set vertex buffer
	constexpr UINT stride{ sizeof( Vertex ) };
	constexpr UINT offset{};
	pDeviceContext->IASetVertexBuffers( 0, 1, &m_pVertexBuffer, &stride, &offset );

	// 4. Set index buffer
	pDeviceContext->IASetIndexBuffer( m_pIndexBuffer, DXGI_FORMAT_R32_UINT, 0 );

	// 5. Draw
	D3DX11_TECHNIQUE_DESC techDesc{};
	m_Effect.GetTechniquePtr()->GetDesc( &techDesc );
	for ( UINT passIdx{}; passIdx < techDesc.Passes; ++passIdx )
	{
		m_Effect.GetTechniquePtr()->GetPassByIndex( passIdx )->Apply( 0, pDeviceContext );
		pDeviceContext->DrawIndexed( m_IndexCount, 0, 0 );
	}
}

void Mesh::CycleFilteringMode()
{
	m_Effect.CycleFilteringMode();
}

void Mesh::ApplyMatrix( const Matrix& action )
{
	m_WorldMatrix = action * m_WorldMatrix;
}

void Mesh::SetWorldViewProjection( const Vector3& o, const Matrix& v, const Matrix& p )
{
	m_Effect.SetWorldViewProjection( m_WorldMatrix * ( v * p ) );
	m_Effect.SetWorld( m_WorldMatrix );
	m_Effect.SetCameraOrigin( o );
}

void Mesh::SetWorld( const Matrix& w )
{
	m_WorldMatrix = w;
	m_Effect.SetWorld( m_WorldMatrix );
}

ID3D11Buffer* Mesh::GetVertexBufferPtr() const
{
	return m_pVertexBuffer;
}

ID3D11Buffer* Mesh::GetIndexBufferPtr() const
{
	return m_pIndexBuffer;
}

Effect* Mesh::GetEffectPtr()
{
	return &m_Effect;
}

uint32_t Mesh::GetVertexCount() const
{
	return m_VertexCount;
}

uint32_t Mesh::GetIndexCount() const
{
	return m_IndexCount;
}
TransparentMesh::TransparentMesh( ID3D11Device* pDevice,
								  const std::vector<Vertex>& vertices,
								  const std::vector<UINT>& indices,
								  D3D11_PRIMITIVE_TOPOLOGY topology,
								  const std::wstring& effectPath,
								  const std::string& diffuseMapPath )
	: m_Topology( topology )
	, m_Effect( pDevice, effectPath )
	, m_DiffuseMap( pDevice, diffuseMapPath )
{
	if ( vertices.size() == 0 )
	{
		throw error::mesh::BufferIsEmpty();
	}

	if ( indices.size() == 0 )
	{
		throw error::mesh::BufferIsEmpty();
	}
	m_VertexCount = vertices.size();
	m_IndexCount = indices.size();

	// Create Vertex Buffer
	D3D11_BUFFER_DESC vertexBufferDesc{};
	vertexBufferDesc.Usage = D3D11_USAGE_IMMUTABLE;
	vertexBufferDesc.ByteWidth = sizeof( Vertex ) * m_VertexCount;
	vertexBufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;

	D3D11_SUBRESOURCE_DATA vertexData{};
	vertexData.pSysMem = vertices.data();

	HRESULT result{ pDevice->CreateBuffer( &vertexBufferDesc, &vertexData, &m_pVertexBuffer ) };
	if ( FAILED( result ) )
	{
		throw static_cast<int>( result );
	}
	//

	// Create Index Buffer
	D3D11_BUFFER_DESC indexBufferDesc{};
	indexBufferDesc.Usage = D3D11_USAGE_IMMUTABLE;
	indexBufferDesc.ByteWidth = sizeof( UINT ) * m_IndexCount;
	indexBufferDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;

	D3D11_SUBRESOURCE_DATA indexData{};
	indexData.pSysMem = indices.data();

	result = pDevice->CreateBuffer( &indexBufferDesc, &indexData, &m_pIndexBuffer );
	if ( FAILED( result ) )
	{
		throw static_cast<int>( result );
	}
	//

	// Pass texture view to effect
	m_Effect.SetDiffuseMap( m_DiffuseMap );
	//
}

const std::vector<Vertex>& Mesh::GetVertices() const
{
	return m_Vertices;
}

const std::vector<UINT>& Mesh::GetIndices() const
{
	return m_Indices;
}

const Matrix& Mesh::GetWorld() const
{
	return m_WorldMatrix;
}

D3D11_PRIMITIVE_TOPOLOGY Mesh::GetTopology() const
{
	return m_Topology;
}

const Texture& Mesh::GetDiffuseMap() const
{
	return m_DiffuseMap;
}

const Texture& Mesh::GetNormalMap() const
{
	return m_NormalMap;
}

const Texture& Mesh::GetSpecularMap() const
{
	return m_SpecularMap;
}

const Texture& Mesh::GetGlossMap() const
{
	return m_GlossMap;
}

TransparentMesh::TransparentMesh( TransparentMesh&& rhs )
{
	if ( this == &rhs )
	{
		return;
	}

	m_VertexCount = rhs.m_VertexCount;
	m_IndexCount = rhs.m_IndexCount;
	m_Topology = rhs.m_Topology;
	m_WorldMatrix = rhs.m_WorldMatrix;

	m_pVertexBuffer = rhs.m_pVertexBuffer;
	rhs.m_pVertexBuffer = nullptr;

	m_pIndexBuffer = rhs.m_pIndexBuffer;
	rhs.m_pIndexBuffer = nullptr;

	m_Effect = std::move( rhs.m_Effect );
	m_DiffuseMap = std::move( rhs.m_DiffuseMap );
}

TransparentMesh& TransparentMesh::operator=( TransparentMesh&& rhs )
{
	if ( this == &rhs )
	{
		return *this;
	}

	m_VertexCount = rhs.m_VertexCount;
	m_IndexCount = rhs.m_IndexCount;
	m_Topology = rhs.m_Topology;
	m_WorldMatrix = rhs.m_WorldMatrix;

	m_pVertexBuffer = rhs.m_pVertexBuffer;
	rhs.m_pVertexBuffer = nullptr;

	m_pIndexBuffer = rhs.m_pIndexBuffer;
	rhs.m_pIndexBuffer = nullptr;

	m_Effect = std::move( rhs.m_Effect );
	m_DiffuseMap = std::move( rhs.m_DiffuseMap );

	return *this;
}

TransparentMesh::~TransparentMesh() noexcept
{
	if ( m_pVertexBuffer )
	{
		m_pVertexBuffer->Release();
	}

	if ( m_pIndexBuffer )
	{
		m_pIndexBuffer->Release();
	}
}

void TransparentMesh::Draw( ID3D11DeviceContext* pDeviceContext ) const
{
	// 1. Set primitive topology
	pDeviceContext->IASetPrimitiveTopology( m_Topology );

	// 2. Set input layout
	pDeviceContext->IASetInputLayout( m_Effect.GetInputLayoutPtr() );

	// 3. Set vertex buffer
	constexpr UINT stride{ sizeof( Vertex ) };
	constexpr UINT offset{};
	pDeviceContext->IASetVertexBuffers( 0, 1, &m_pVertexBuffer, &stride, &offset );

	// 4. Set index buffer
	pDeviceContext->IASetIndexBuffer( m_pIndexBuffer, DXGI_FORMAT_R32_UINT, 0 );

	// 5. Draw
	D3DX11_TECHNIQUE_DESC techDesc{};
	m_Effect.GetTechniquePtr()->GetDesc( &techDesc );
	for ( UINT passIdx{}; passIdx < techDesc.Passes; ++passIdx )
	{
		m_Effect.GetTechniquePtr()->GetPassByIndex( passIdx )->Apply( 0, pDeviceContext );
		pDeviceContext->DrawIndexed( m_IndexCount, 0, 0 );
	}
}

void TransparentMesh::CycleFilteringMode()
{
	m_Effect.CycleFilteringMode();
}

void TransparentMesh::ApplyMatrix( const Matrix& action )
{
	m_WorldMatrix = action * m_WorldMatrix;
}

void TransparentMesh::SetWorldViewProjection( const Matrix& v, const Matrix& p )
{
	m_Effect.SetWorldViewProjection( m_WorldMatrix * ( v * p ) );
}

void TransparentMesh::SetWorld( const Matrix& w )
{
	m_WorldMatrix = w;
}

ID3D11Buffer* TransparentMesh::GetVertexBufferPtr() const
{
	return m_pVertexBuffer;
}

ID3D11Buffer* TransparentMesh::GetIndexBufferPtr() const
{
	return m_pIndexBuffer;
}

TransparentEffect* TransparentMesh::GetEffectPtr()
{
	return &m_Effect;
}

uint32_t TransparentMesh::GetVertexCount() const
{
	return m_VertexCount;
}

uint32_t TransparentMesh::GetIndexCount() const
{
	return m_IndexCount;
}
} // namespace dae
