#include "Shading.h"
#include <iostream>

// Why does Michael Soft III do this to me?
#undef max

namespace dae
{
ColorRGB GetPixelColor( const VertexOut& pixelVertex,
						const Texture& diffuseMap,
						const Texture& normalMap,
						const Texture& specularMap,
						const Texture& glossMap,
						const Camera& camera,
						const Vector3& lightDirection,
						const LightingMode& lightingMode,
						bool useNormalMap )
{
	const ColorRGB diffuseColor{ diffuseMap.Sample( pixelVertex.uv ) };

	if ( lightDirection == Vector3{ 0.f, 0.f, 0.f } )
	{
		return diffuseColor;
	}

	Vector3 sampledNormal{};
	if ( useNormalMap )
	{
		const Vector3 binormal{ Vector3::Cross( pixelVertex.normal, pixelVertex.tangent ).Normalized() };
		const Matrix tangentAxisSpace{ pixelVertex.tangent, binormal, pixelVertex.normal, {} };
		ColorRGB sampledNormalColor{ ( normalMap.Sample( pixelVertex.uv ) ) };
		sampledNormal = { sampledNormalColor.r, sampledNormalColor.g, sampledNormalColor.b };
		sampledNormal = ( sampledNormal * 2.f ) - Vector3{ 1.f, 1.f, 1.f };
		sampledNormal = tangentAxisSpace.TransformVector( sampledNormal );
		sampledNormal.Normalize();
	}
	else
	{
		sampledNormal = pixelVertex.normal;
	}

	const ColorRGB sampledSpecularity{ specularMap.Sample( pixelVertex.uv ) };
	const float sampledGloss{ glossMap.Sample( pixelVertex.uv ).r }; // Assuming map is greyscale

	const Vector3 toCameraDir{ Vector3( pixelVertex.worldPosition, camera.GetPosition() ).Normalized() };

	ColorRGB finalColor{};

	// List of hardcoded values because
	constexpr float diffuseReflectance{ 7.f }; // Hardcoded to make up for lack of lights
	constexpr float shininess{ 25.f };
	constexpr ColorRGB ambientLight{ 0.03f, 0.03f, 0.03f };

	const float observedArea{ lightUtils::GetObservedArea( lightDirection, sampledNormal ) };
	const ColorRGB lambertDiffuse{ ( diffuseColor * diffuseReflectance ) / PI };
	const ColorRGB phongSpecular{ lightUtils::GetPhong(
		sampledSpecularity, sampledGloss * shininess, lightDirection, toCameraDir, sampledNormal ) };
	const ColorRGB brdf{ lambertDiffuse + phongSpecular + ambientLight };

	switch ( lightingMode )
	{
	case LightingMode::observedArea:
		finalColor += ColorRGB{ observedArea, observedArea, observedArea };
		break;

	case LightingMode::diffuse:
		finalColor += lambertDiffuse;
		break;

	case LightingMode::specular:
		finalColor += phongSpecular;
		break;

	case LightingMode::combined:
		finalColor += observedArea * brdf;
		break;

	default:
		break;
	}

	finalColor.MaxToOne();

	return finalColor;
}

namespace lightUtils
{
float GetObservedArea( const Vector3& lightDirection, const Vector3& normal )
{
	return std::max( Vector3::Dot( normal, -lightDirection ), 0.f );
}

ColorRGB GetPhong( ColorRGB specularReflectance,
				   float phongExponent,
				   const Vector3& lightIncomingDir,
				   const Vector3& toCameraDir,
				   const Vector3& normal )
{
	const Vector3 reflectLight{ Vector3::Reflect( -lightIncomingDir, normal ) };
	const float closingFactor{ std::max( Vector3::Dot( reflectLight, -toCameraDir ), 0.f ) };
	return specularReflectance * pow( closingFactor, phongExponent );
}
} // namespace lightUtils
} // namespace dae
