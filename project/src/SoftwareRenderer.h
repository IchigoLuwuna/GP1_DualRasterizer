#ifndef SOFTWARE_RENDERER_H
#define SOFTWARE_RENDERER_H

#include "Camera.h"
#include "Shading.h"
#include "Structs.h"

struct SDL_Window;
struct SDL_Surface;

namespace dae
{
class Texture;
class Mesh;
struct Vertex;
class Timer;
class Scene;

class SoftwareRenderer final
{
public:
	SoftwareRenderer( SDL_Window* pWindow );
	~SoftwareRenderer();

	SoftwareRenderer( const SoftwareRenderer& ) = delete;
	SoftwareRenderer( SoftwareRenderer&& ) noexcept = delete;
	SoftwareRenderer& operator=( const SoftwareRenderer& ) = delete;
	SoftwareRenderer& operator=( SoftwareRenderer&& ) noexcept = delete;

	void Update( const Timer& timer );
	void Render( const Scene* pScene );

	bool SaveBufferToImage() const;

private:
	SDL_Window* m_pWindow{};

	SDL_Surface* m_pFrontBuffer{ nullptr };
	SDL_Surface* m_pBackBuffer{ nullptr };
	uint32_t* m_pBackBufferPixels{};

	std::vector<float> m_DepthBufferPixels{};
	std::vector<std::pair<bool, VertexOut>> m_PixelAttributeBuffer{};

	std::vector<VertexOut> m_VertexOutBuffer{};

	int m_Width{};
	int m_Height{};

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
};
} // namespace dae

#endif
