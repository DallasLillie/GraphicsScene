#include "LightingCalculations.hlsli"

#define NUM_LIGHTS 3

cbuffer LightsBuffer : register(b0)
{
	LIGHT lights[NUM_LIGHTS];
};

cbuffer SpecularBufferCam : register(b1)
{
	float4 cameraPosition;
}

// Per-pixel color data passed through the pixel shader.
struct PixelShaderInput
{
	float4 pos : SV_POSITION;
	float3 wPos : WPOSITION;
	float2 texCoord : TEXCOORD;
	float4 tangent : TANGENT;
	float4 biTangent :BTANGENT;
	float3 normal : NORMAL;
};

texture2D baseTexture : register(t0);
texture2D normalTexture : register(t1);

SamplerState filter : register(s0);

// A pass-through function for the (interpolated) color data.
float4 main(PixelShaderInput input) : SV_TARGET
{
	float4 baseColor = baseTexture.Sample(filter, input.texCoord);

	float4 newNormal = normalTexture.Sample(filter, input.texCoord);

	newNormal = (newNormal*2.0f) - 1.0f;
	input.normal = normalize(input.normal);
	input.tangent = normalize(input.tangent);
	input.biTangent = normalize(input.biTangent);

	float3x3 TBNMatrix;
	TBNMatrix[0] = input.tangent.xyz;
	TBNMatrix[1] = input.biTangent.xyz;
	TBNMatrix[2] = input.normal.xyz;

	newNormal = float4(mul(newNormal.xyz, TBNMatrix),0.0f);

	float3 lightColor;
	lightColor.x = 0;
	lightColor.y = 0;
	lightColor.z = 0;

	for (int i = 0; i < NUM_LIGHTS; ++i)
	{
		switch (uint(lights[i].position.w))
		{
		case 0:
			lightColor += CalculateDirectionalLighting(lights[i], newNormal.xyz);
			break;
		case 1:
			lightColor += CalculatePointLighting(lights[i], input.wPos);
			break;
		case 2:
			lightColor += CalculateConeLighting(lights[i], input.wPos, newNormal.xyz);
			break;
		}
		float3 specHighLight = CalculateSpecularLighting(lights[i], input.wPos, cameraPosition, input.normal, lights[i].color.xyz);
		lightColor += specHighLight;
	}

	return saturate(baseColor *float4(lightColor, 1.0f));
}
