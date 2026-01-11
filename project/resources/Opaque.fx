// -------------
// | Constants |
// -------------
static const float pi = 3.14159265f;
static const float lightIntensity = 7.f;
static const float3 ambientLight = float3(0.025f, 0.025f, 0.025f);
static const float shininess = 25.f;
static const float3 lightDirection = {0.577f, -0.577f, 0.577f};

// -----------------
// | Scene Globals |
// -----------------
// Rasterizer
RasterizerState gRasterizerState
{
	CullMode = back;
	FrontCounterClockWise = false; // default
};
BlendState gBlendState
{
	BlendEnable[0] = false;
};
DepthStencilState gDepthStencilState
{
	DepthEnable = true; // enable read
	DepthWriteMask = all; // enable write
	DepthFunc = less;
	StencilEnable = false;
};

// Camera & Worldspace
float4x4 gWorldViewProj : WorldViewProjection;
float4x4 gWorld : World;
float4 gCameraOrigin : CameraOrigin;

// Textures
Texture2D gDiffuseMap : DiffuseMap;
Texture2D gNormalMap : NormalMap;
Texture2D gSpecularMap : SpecularMap;
Texture2D gGlossMap : GlossMap;
SamplerState gSampler : Sampler;

// -----------
// | Structs |
// -----------
struct VS_INPUT
{
	float3 Position : POSITION;
	float3 Color : COLOR;
	float2 UV : TEXCOORD;
	float3 Normal : NORMAL;
	float3 Tangent : TANGENT;
};

struct VS_OUTPUT
{
	float4 Position : SV_POSITION;
	float4 WorldPosition : WORLD;
	float3 Color : COLOR;
	float2 UV : TEXCOORD;
	float3 Normal : NORMAL;
	float3 Tangent : TANGENT;
};

// ---------
// | Extra |
// ---------
float3 GetToCamera(float3 origin)
{
	return normalize(gCameraOrigin.xyz - origin);
}

float3 GetSampledNormal(float2 uv, float3 normal, float3 tangent)
{
	float3 sampledNormal = gNormalMap.Sample(gSampler, uv).rgb;
	sampledNormal = 2.f * sampledNormal - float3(1.f, 1.f, 1.f); // remap normal

	const float3 binormal = cross(normal, tangent);
	const float3x3 TBN = float3x3(tangent, binormal, normal);

	return mul(sampledNormal, TBN);
}

float3 MultColor(float3 a, float3 b)
{
	return float3(a.r * b.r, a.g * b.g, a.b * b.b);
}

// ------------
// | Lighting |
// ------------
float CalculateOA(float3 normal)
{
	return dot(normal, -lightDirection);
}

float3 CalculateLambert(float3 diffuseColor, float diffuseReflectance)
{
	return diffuseColor * diffuseReflectance / pi;
}

float3 CalculatePhong(float3 specularReflect, float phongExponent, float3 lightInDir, float3 camToDir, float3 normal)
{
	const float3 reflectLight = reflect(lightInDir, normal);
	const float closingFactor = max(dot(reflectLight, camToDir), 0.f);
	return specularReflect * pow(closingFactor, phongExponent);
}

// -----------
// | Shaders |
// -----------
// Vertex Shader
VS_OUTPUT VtxShader(VS_INPUT input)
{
	VS_OUTPUT output = (VS_OUTPUT)0;
	output.Position = mul( float4( input.Position, 1.f ), gWorldViewProj );
	output.WorldPosition = mul( float4( input.Position, 1.f ), gWorld );
	output.Color = input.Color;
	output.UV = input.UV;
	output.Normal = normalize( mul( input.Normal, (float3x3)gWorld ).xyz );
	output.Tangent = normalize( mul( input.Tangent, (float3x3)gWorld ).xyz );
	return output;
}

// Pixel Shader
float4 PxlShader(VS_OUTPUT input) : SV_TARGET
{
	// Calculate view direction
	const float3 originToCamera = GetToCamera(input.WorldPosition.xyz);

	// Get normal from normal map
	float3 normal = GetSampledNormal(input.UV, input.Normal, input.Tangent);

	// Calculate diffuse
	const float3 sampledColor = gDiffuseMap.Sample(gSampler, input.UV).rgb;
	const float3 lambertDiffuse = CalculateLambert(sampledColor, lightIntensity);

	// Calculate phong
	const float3 sampledSpecular = gSpecularMap.Sample(gSampler, input.UV).rgb;
	const float sampledGloss = gGlossMap.Sample(gSampler, input.UV).r;
	const float phongExponent = sampledGloss * shininess;
	const float3 phongSpecular = CalculatePhong(sampledSpecular, phongExponent, lightDirection, originToCamera, normal);

	// Calculate final color
	const float3 brdf = lambertDiffuse + phongSpecular + ambientLight;
	float3 finalColor = brdf * CalculateOA(normal);
	finalColor = saturate(finalColor);

	return float4(finalColor, 1.f);
}

// --------------
// | Techniques |
// --------------
// Technique
technique11 DefaultTechnique
{
	pass P0
	{
		SetRasterizerState(gRasterizerState);
		SetDepthStencilState(gDepthStencilState, 0);
		SetBlendState(gBlendState, float4(0.f, 0.f, 0.f, 0.f), -1);
		SetVertexShader( CompileShader( vs_5_0, VtxShader() ) );
		SetGeometryShader( NULL );
		SetPixelShader( CompileShader( ps_5_0, PxlShader() ) );
	}
}
