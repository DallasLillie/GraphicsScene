cbuffer ViewProjectionConstantBuffer : register(b0)
{
	matrix view[2];
	matrix projection[2];
};

cbuffer ViewProjectionLightBuffer : register(b1)
{
	matrix viewL;
	matrix projectionL;
};

struct GSInput
{
	float4 pos : SV_POSITION;
	float2 texCoord : TEXCOORD;
	float4 tangent : TANGENT;
	float4 biTangent :BTANGENT;
	float3 normal : NORMAL;
};

struct GSOutput
{
	float4 pos : SV_POSITION;
	float3 wPos : WPOSITION;
	float2 texCoord : TEXCOORD0;
	float4 tangent : TANGENT;
	float4 biTangent :BTANGENT;
	float3 normal : NORMAL;
	float4 projTex : TEXCOORD1;
	uint viewport : SV_ViewportArrayIndex;
};


[maxvertexcount(8)]
void main(
	point GSInput input[1],
	inout TriangleStream< GSOutput > output
)
{
	for (unsigned int i = 0; i < 2; ++i)
	{
		GSOutput corner1 = { float4(-10.0f,  0.0f,  10.0f,1.0f),float3(-10.0f,  0.0f,  10.0f), float2(0.0f,0.0f),float4(-1.0f,0.0f,0.0f,0.0f),float4(0.0f,0.0f,1.0f,0.0f),float3(0.0f,1.0f,0.0f),float4(0.0f,0.0f,0.0f,0.0f),i };
		GSOutput corner2 = { float4(10.0f,  0.0f, 10.0f,1.0f),float3(10.0f,  0.0f, 10.0f), float2(20.0f,0.0f) ,float4(-1.0f,0.0f,0.0f,0.0f),  float4(0.0f,0.0f,1.0f,0.0f),float3(0.0f,1.0f,0.0f),float4(0.0f,0.0f,0.0f,0.0f),i };
		GSOutput corner3 = { float4(-10.0f,  0.0f, -10.0f,1.0f),float3(-10.0f,  0.0f, -10.0f),float2(0.0f,20.0f),float4(-1.0f,0.0f,0.0f,0.0f),float4(0.0f,0.0f,1.0f,0.0f),float3(0.0f,1.0f,0.0f),float4(0.0f,0.0f,0.0f,0.0f),i };
		GSOutput corner4 = { float4(10.0f,  0.0f, -10.0f,1.0f),float3(10.0f,  0.0f, -10.0f),float2(20.0f,20.0f),float4(-1.0f,0.0f,0.0f,0.0f), float4(0.0f,0.0f,1.0f,0.0f),float3(0.0f,1.0f,0.0f),float4(0.0f,0.0f,0.0f,0.0f),i };
		
		corner1.projTex = mul(corner1.pos, viewL);
		corner1.projTex = mul(corner1.projTex, projectionL);

		corner2.projTex = mul(corner2.pos, viewL);
		corner2.projTex = mul(corner2.projTex, projectionL);

		corner3.projTex = mul(corner3.pos, viewL);
		corner3.projTex = mul(corner3.projTex, projectionL);

		corner4.projTex = mul(corner4.pos, viewL);
		corner4.projTex = mul(corner4.projTex, projectionL);



		corner1.pos = mul(corner1.pos, view[i]);
		corner1.pos = mul(corner1.pos, projection[i]);

		corner2.pos = mul(corner2.pos, view[i]);
		corner2.pos = mul(corner2.pos, projection[i]);

		corner3.pos = mul(corner3.pos, view[i]);
		corner3.pos = mul(corner3.pos, projection[i]);

		corner4.pos = mul(corner4.pos, view[i]);
		corner4.pos = mul(corner4.pos, projection[i]);

		//corner1.projTex = corner1.pos;
		//corner2.projTex = corner2.pos;
		//corner3.projTex = corner3.pos;
		//corner4.projTex = corner4.pos;

		output.Append(corner1);
		output.Append(corner2);
		output.Append(corner3);
		output.Append(corner4);
	}
}