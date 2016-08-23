
struct PixelShaderInput
{
	float4 pos : SV_POSITION;
	float2 texCoords : TEXCOORD;
};

cbuffer ScreenEffect : register (b0)
{
	float4 lossColor;
};

texture2D screenTexture : register(t0);
SamplerState filter : register(s0);

float4 main(PixelShaderInput input) : SV_TARGET
{
	float4 screenColor = screenTexture.Sample(filter,input.texCoords);
	screenColor -= lossColor;
	return screenColor;
}