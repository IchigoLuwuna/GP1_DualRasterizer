#ifndef SHADING_H
#define SHADING_H
#include "Camera.h"
#include "ColorRGB.h"
#include "Structs.h"
#include "Mesh.h"

// Everything related to shading

enum class LightingMode
{
	observedArea,
	diffuse,
	specular,
	combined,
	count,
};

namespace dae
{
enum class LightType
{
	point,
	directional
};

ColorRGB GetPixelColor( const VertexOut& pixelVertex,
						const Texture& diffuseMap,
						const Texture& normalMap,
						const Texture& specularMap,
						const Texture& glossMap,
						const Camera& camera,
						const Vector3& lightDirection,
						const LightingMode& lightingMode,
						bool useNormalMap = true );

namespace lightUtils
{
float GetObservedArea( const Vector3& lightDirection, const Vector3& normal );
ColorRGB GetPhong( ColorRGB specularReflectance,
				   float phongExponent,
				   const Vector3& lightIncomingDir,
				   const Vector3& toCameraDir,
				   const Vector3& normal );
} // namespace lightUtils
} // namespace dae

#endif
