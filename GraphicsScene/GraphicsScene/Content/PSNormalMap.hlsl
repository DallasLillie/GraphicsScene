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
	float4 tangent : TANGENT;
	float4 biTangent :BTANGENT;
	float3 normal : NORMAL;
	float4 projTex : TEXCOORD1;
};

texture2D baseTexture : register(t0);
texture2D normalTexture : register(t1);
texture2D shadowMap : register(t2);
SamplerState filter : register(s0);
SamplerComparisonState comp0Filter : register(s1);

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

	float3 addColor = float3(0.0f, 0.0f, 0.0f);

	newNormal = float4(mul(newNormal.xyz, TBNMatrix),0.0f);


	float3 lightColor;
	lightColor.x = 0;
	lightColor.y = 0;
	lightColor.z = 0;

	//TODO: Less branching (AFTER Project Task)
	for (int i = 0; i < NUM_LIGHTS; ++i)
	{
		switch (uint(lights[i].position.w))
		{
		case 0:
		{
			if (lights[i].ratio.w)
			{
				addColor = float3(0.0f, 0.0f, 0.0f);
				addColor += CalculateDirectionalLighting(lights[i], newNormal.xyz);
				float3 specHighLight = CalculateSpecularLighting(lights[i], input.wPos, cameraPosition, newNormal.xyz, lights[i].color.xyz);
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
				addColor += CalculatePointLighting(lights[i],newNormal.xyz,input.wPos);
				float3 specHighLight = CalculateSpecularLighting(lights[i], input.wPos, cameraPosition, newNormal.xyz, lights[i].color.xyz);
				addColor += specHighLight;
				addColor *= CalculatePointAttenuation(lights[i], newNormal.xyz, input.wPos);
				lightColor += saturate(addColor);
			}
			break;
		}
		case 2:
		{
			if (lights[i].ratio.w)
			{
				addColor = float3(0.0f, 0.0f, 0.0f);
				addColor += CalculateConeLighting(lights[i], input.wPos, newNormal.xyz);
				float3 specHighLight = CalculateSpecularLighting(lights[i], input.wPos, cameraPosition, newNormal.xyz, lights[i].color.xyz);
				addColor += specHighLight;
				addColor *= CalculateConeFalloff(lights[i], input.wPos);
				lightColor += saturate(addColor);
			}
			break;
		}
		}
	}


	//input.projTex.xyz /= input.projTex.w;
	//input.projTex.xy = (input.projTex.xy + 1)*0.5f;
	//input.projTex.y = 1.0f-input.projTex.y;

	//float ourDepth = input.projTex.z;
	//float sampleDepth = shadowMap.Sample(filter, input.projTex.xy);
	//float depthBias = 0.0005f;

	//float shadowFactor = (ourDepth <= sampleDepth+depthBias);


	//PCF
	input.projTex.xyz /= input.projTex.w;
	input.projTex.xy = (input.projTex.xy + 1)*0.5f;
	input.projTex.y = 1.0f-input.projTex.y;
	float ourDepth = input.projTex.z;
	float depthBias = 0.0005f;

	float dx = (1.0f/2048.0f)*1.5f;
	float percentLit = 0.0f;
	float2 offsets[9] =
	{
		float2(-dx,-dx),float2(0.0f,-dx),float2(dx,-dx),
		float2(-dx,0.0f),float2(0.0f,0.0f),float2(dx,0.0f),
		float2(-dx,dx),float2(0.0f,dx),float2(dx,dx)
	};



	for (unsigned int i = 0; i < 9; ++i)
	{
		percentLit += shadowMap.SampleCmpLevelZero(comp0Filter, input.projTex.xy + offsets[i].xy, ourDepth-depthBias, 0).r;
	}
	percentLit /= 9.0f;

	float shadowFactor = percentLit;

	return saturate(baseColor *float4((lightColor*shadowFactor)+0.1f, 1.0f));
}
