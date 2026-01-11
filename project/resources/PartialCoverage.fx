// -----------------
// | Scene Globals |
// -----------------
// Rasterizer
RasterizerState gRasterizerState
{
	CullMode = none;
	FrontCounterClockWise = false; // default
};
BlendState gBlendState
{
	BlendEnable[0] = true;
	SrcBlend = src_alpha;
	DestBlend = inv_src_alpha;
	BlendOp = add;
	SrcBlendAlpha = zero;
	DestBlendAlpha = zero;
	BlendOpAlpha = add;
	RenderTargetWriteMask[0] = 0x0F;
};
DepthStencilState gDepthStencilState
{
	DepthEnable = true; // enable read
	DepthWriteMask = zero; // disable write
	DepthFunc = less;
	StencilEnable = false;
};

// Camera & Worldspace
float4x4 gWorldViewProj : WorldViewProjection;

// Textures
Texture2D gDiffuseMap : DiffuseMap;
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
	float2 UV : TEXCOORD;
};

// -----------
// | Shaders |
// -----------
// Vertex Shader
VS_OUTPUT VtxShader(VS_INPUT input)
{
	VS_OUTPUT output = (VS_OUTPUT)0;
	output.Position = mul( float4( input.Position, 1.f ), gWorldViewProj );
	output.UV = input.UV;
	return output;
}

// Pixel Shader
float4 PxlShader(VS_OUTPUT input) : SV_TARGET
{
	return gDiffuseMap.Sample(gSampler, input.UV);
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
