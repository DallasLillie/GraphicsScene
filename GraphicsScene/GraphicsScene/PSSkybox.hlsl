
struct PixelShaderInput
{
	float4 pos : SV_POSITION;
	float3 texCoord : TEXCOORD;
};

TextureCube skyBox : register(t0);

SamplerState filter : register(s0);

float4 main(PixelShaderInput input) : SV_TARGET
{
	float3 skyboxColor = skyBox.Sample(filter,input.texCoord);

	return float4(skyboxColor, 1.0f);
}