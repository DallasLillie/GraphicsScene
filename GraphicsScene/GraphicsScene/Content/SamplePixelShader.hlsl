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
TextureCube skyBox : register(t1);

SamplerState filter : register(s0);

// A pass-through function for the (interpolated) color data.
float4 main(PixelShaderInput input) : SV_TARGET
{
	float4 baseColor = baseTexture.Sample(filter, input.texCoord);

	float3 lightColor;
	lightColor.x = 0;
	lightColor.y = 0;
	lightColor.z = 0;

	for (int i = 0; i < NUM_LIGHTS; ++i)
	{
		switch (uint(lights[i].position.w))
		{
		case 0:
		{
			lightColor += CalculateDirectionalLighting(lights[i], input.normal);
			float3 specHighLight = CalculateSpecularLighting(lights[i], input.wPos, cameraPosition, input.normal, lights[i].color.xyz);
			lightColor += specHighLight;
			break;
		}
		case 1:
		{
			lightColor += CalculatePointLighting(lights[i], input.normal,input.wPos);
			float3 specHighLight = CalculateSpecularLighting(lights[i], input.wPos, cameraPosition, input.normal, lights[i].color.xyz);
			lightColor += specHighLight;
			//lightColor *= CalculateSpotAttenuation(lights[i], input.normal, input.wPos);
			break;
		}
		case 2:
		{
			lightColor += CalculateConeLighting(lights[i], input.wPos, input.normal);
			float3 specHighLight = CalculateSpecularLighting(lights[i], input.wPos, cameraPosition, input.normal, lights[i].color.xyz);
			lightColor += (specHighLight);
			//lightColor *= CalculateConeFalloff(lights[i], input.wPos);
			break;
		}
		}
	}


	return saturate(baseColor *float4(lightColor, 1.0f));
}
