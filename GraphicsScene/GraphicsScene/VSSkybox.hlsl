cbuffer ModelConstantBuffer : register(b0)
{
	matrix model;
};

struct VertexShaderInput
{
	float3 pos : POSITION;
	float2 texCoord : TEXCOORD;
	float4 tangent : TANGENT;
	float3 normal : NORMAL;
};

struct VertexShaderOutput
{
	float4 pos : SV_POSITION;
	float3 texCoord : TEXCOORD;
};

VertexShaderOutput main(VertexShaderInput input)
{
	VertexShaderOutput output = (VertexShaderOutput)0;
	float4 pos = float4(input.pos, 1.0f);
	output.texCoord = pos.xyz;

	pos = mul(pos, model);
	output.pos = pos;

	return output;
}