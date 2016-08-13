cbuffer ViewProjectionConstantBuffer : register(b0)
{
	//matrix model;
	matrix view[2];
	matrix projection[2];
};

struct GSOutput
{
	float4 pos : SV_POSITION;
	float3 wPos : WPOSITION;
	float2 texCoord : TEXCOORD;
	float4 tangent : TANGENT;
	float4 biTangent :BTANGENT;
	float3 normal : NORMAL;
	uint viewport : SV_ViewportArrayIndex;
};

struct GSInput
{
	float4 pos : SV_POSITION;
	//float3 wPos : WPOSITION;
	float2 texCoord : TEXCOORD;
	float4 tangent : TANGENT;
	float4 biTangent :BTANGENT;
	float3 normal : NORMAL;
};

[maxvertexcount(8)]
void main(
	point GSInput input[1],
	inout TriangleStream< GSOutput > output
)
{
	for (unsigned int i = 0; i < 2; ++i)
	{
		GSOutput corner1 = { float4(-50.0f,  0.0f,  50.0f,1.0f),float3(-50.0f,  0.0f,  50.0f), float2(0.0f,0.0f),float4(-1.0f,0.0f,0.0f,0.0f),float4(0.0f,0.0f,1.0f,0.0f),float3(0.0f,1.0f,0.0f),i };
		GSOutput corner2 = { float4(50.0f,  0.0f, 50.0f,1.0f),float3(50.0f,  0.0f, 50.0f), float2(100.0f,0.0f) ,float4(-1.0f,0.0f,0.0f,0.0f),float4(0.0f,0.0f,1.0f,0.0f),float3(0.0f,1.0f,0.0f),i };
		GSOutput corner3 = { float4(-50.0f,  0.0f, -50.0f,1.0f),float3(-50.0f,  0.0f, -50.0f),float2(0.0f,100.0f),float4(-1.0f,0.0f,0.0f,0.0f),float4(0.0f,0.0f,1.0f,0.0f),float3(0.0f,1.0f,0.0f),i };
		GSOutput corner4 = { float4(50.0f,  0.0f, -50.0f,1.0f),float3(50.0f,  0.0f, -50.0f),float2(100.0f,100.0f),float4(-1.0f,0.0f,0.0f,0.0f),float4(0.0f,0.0f,1.0f,0.0f),float3(0.0f,1.0f,0.0f),i };

		corner1.pos = mul(corner1.pos, view[i]);
		corner1.pos = mul(corner1.pos, projection[i]);

		corner2.pos = mul(corner2.pos, view[i]);
		corner2.pos = mul(corner2.pos, projection[i]);

		corner3.pos = mul(corner3.pos, view[i]);
		corner3.pos = mul(corner3.pos, projection[i]);

		corner4.pos = mul(corner4.pos, view[i]);
		corner4.pos = mul(corner4.pos, projection[i]);


		output.Append(corner1);
		output.Append(corner2);
		output.Append(corner3);
		output.Append(corner4);
	}
}