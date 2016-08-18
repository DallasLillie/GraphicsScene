#include "LightingCalculations.hlsli"

// A constant buffer that stores the three basic column-major matrices for composing geometry.
cbuffer ModelViewProjectionConstantBuffer : register(b0)
{
	matrix model;
	matrix view;
	matrix projection;
};

// Per-vertex data used as input to the vertex shader.
struct VertexShaderInput
{
	float3 pos : POSITION;
	//float3 wPos : WPOSITION;
	float2 texCoord : TEXCOORD;
	float4 tangent : TANGENT;
	float3 normal : NORMAL;
};

// Per-pixel color data passed through the pixel shader.
struct VertexShaderOutput
{
	float4 pos : SV_POSITION;
	float3 wPos : WPOSITION;
	float2 texCoord : TEXCOORD0;
	float4 tangent : TANGENT;
	float4 biTangent :BTANGENT;
	float3 normal : NORMAL;
	float4 projTex : TEXCOORD1;
};

// Simple shader to do vertex processing on the GPU.
VertexShaderOutput main(VertexShaderInput input)
{
	VertexShaderOutput output = (VertexShaderOutput)0;
	float4 pos = float4(input.pos, 1.0f);
	float4 normal = float4(input.normal, 0.0f);
	float4 tangent = input.tangent;

	// Transform the vertex position into projected space.
	pos = mul(pos, model);
	normal = mul(normal, model);
	output.tangent = mul(float4(tangent.xyz, 0.0f), model);
	//output.tangent = mul(float4(tangent.xyz*tangent.w, 0.0f), model);
	output.biTangent = mul(float4(cross(normal.xyz, tangent.xyz), 0.0f), model);

	output.wPos = pos;

	pos = mul(pos, view);
	pos = mul(pos, projection);

	output.pos = pos;
	output.normal = normal;
	output.texCoord = input.texCoord;

	return output;
}
