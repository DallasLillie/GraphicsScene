// Per-pixel color data passed through the pixel shader.
struct PixelShaderInput
{
	float4 pos : SV_POSITION;
	float3 wPos : WPOSITION;
	float2 texCoord : TEXCOORD;
	float4 tangent : TANGENT;
	float3 normal : NORMAL;
};

texture2D baseTexture : register(t0);
//textureCUBE skybox : register(t0);

SamplerState filter : register(s0);

// A pass-through function for the (interpolated) color data.
float4 main(PixelShaderInput input) : SV_TARGET
{
	float4 baseColor = baseTexture.Sample(filter, input.texCoord);
	return baseColor;
	//return float4(input.wPos, 1.0f);
}
