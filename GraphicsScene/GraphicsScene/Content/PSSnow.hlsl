#include "LightingCalculations.hlsli"

#define NUM_LIGHTS 7

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
	float2 texCoord : TEXCOORD0;
	float3 normal : NORMAL;
};

texture2D baseTexture : register(t0);
SamplerState filter : register(s0);

// A pass-through function for the (interpolated) color data.
float4 main(PixelShaderInput input) : SV_TARGET
{
	float4 baseColor = baseTexture.Sample(filter, input.texCoord);
	float3 addColor = float3(0.0f, 0.0f, 0.0f);
	float3 lightColor = float3(0.0f, 0.0f, 0.0f);

	for (int i = 0; i < NUM_LIGHTS; ++i)
	{
		switch (uint(lights[i].position.w))
		{
		case 0:
		{
			if (lights[i].ratio.w)
			{
				addColor = float3(0.0f, 0.0f, 0.0f);
				addColor += CalculateDirectionalLighting(lights[i], input.normal.xyz);
				float3 specHighLight = CalculateSpecularLighting(lights[i], input.wPos, cameraPosition, input.normal.xyz, lights[i].color.xyz);
				addColor += specHighLight;
				lightColor += saturate(addColor);
			}
			break;
		}
		case 1:
		{
			if (lights[i].ratio.w)
			{
				addColor = float3(0.0f, 0.0f, 0.0f);
				addColor += CalculatePointLighting(lights[i], input.normal.xyz,input.wPos);
				float3 specHighLight = CalculateSpecularLighting(lights[i], input.wPos, cameraPosition, input.normal.xyz, lights[i].color.xyz);
				addColor += specHighLight;
				addColor *= CalculatePointAttenuation(lights[i], input.normal.xyz, input.wPos);
				lightColor += saturate(addColor);
			}
			break;
		}
		case 2:
		{
			if (lights[i].ratio.w)
			{
				addColor = float3(0.0f, 0.0f, 0.0f);
				addColor += CalculateConeLighting(lights[i], input.wPos, input.normal.xyz);
				float3 specHighLight = CalculateSpecularLighting(lights[i], input.wPos, cameraPosition, input.normal.xyz, lights[i].color.xyz);
				addColor += specHighLight;
				addColor *= CalculateConeFalloff(lights[i], input.wPos);
				lightColor += saturate(addColor);
			}
			break;
		}
		}
	}

	return saturate(baseColor *float4(lightColor, 1.0f));
}
