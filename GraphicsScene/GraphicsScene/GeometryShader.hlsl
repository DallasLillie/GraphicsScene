cbuffer ModelViewProjectionConstantBuffer : register(b0)
{
	matrix model;
	matrix view;
	matrix projection;
};

struct GSOutput
{
	float4 pos : SV_POSITION;
	float3 wPos : WPOSITION;
	float2 texCoord : TEXCOORD;
	float4 tangent : TANGENT;
	float4 biTangent :BTANGENT;
	float3 normal : NORMAL;
};

struct GSInput
{
	float4 pos : SV_POSITION;
	float3 wPos : WPOSITION;
	float2 texCoord : TEXCOORD;
	float4 tangent : TANGENT;
	float4 biTangent :BTANGENT;
	float3 normal : NORMAL;
};

//TODO: maxvvertex 6 or 4?
[maxvertexcount(4)]
void main(
	point GSInput input[1],
	inout TriangleStream< GSOutput > output
)
{
	//TODO: take in world space
	GSOutput corner1 = { float4(-5.0f,  0.0f,  5.0f,1.0f),float3(-5.0f,  0.0f,  5.0f), float2(0.0f,0.0f),float4(-1.0f,0.0f,0.0f,0.0f),float4(0.0f,0.0f,1.0f,0.0f),float3(0.0f,1.0f,0.0f) };
	GSOutput corner2 = { float4(5.0f,  0.0f, 5.0f,1.0f),float3(5.0f,  0.0f, 5.0f), float2(10.0f,0.0f) ,float4(-1.0f,0.0f,0.0f,0.0f),float4(0.0f,0.0f,1.0f,0.0f),float3(0.0f,1.0f,0.0f) };
	GSOutput corner3 = { float4(-5.0f,  0.0f, -5.0f,1.0f),float3(-5.0f,  0.0f, -5.0f),float2(0.0f,10.0f),float4(-1.0f,0.0f,0.0f,0.0f),float4(0.0f,0.0f,1.0f,0.0f),float3(0.0f,1.0f,0.0f) };
	GSOutput corner4 = { float4(5.0f,  0.0f, -5.0f,1.0f),float3(5.0f,  0.0f, -5.0f),float2(10.0f,10.0f),float4(-1.0f,0.0f,0.0f,0.0f),float4(0.0f,0.0f,1.0f,0.0f),float3(0.0f,1.0f,0.0f) };

	corner1.pos = mul(corner1.pos, view);
	corner1.pos = mul(corner1.pos, projection);

	corner2.pos = mul(corner2.pos, view);
	corner2.pos = mul(corner2.pos, projection);

	corner3.pos = mul(corner3.pos, view);
	corner3.pos = mul(corner3.pos, projection);

	corner4.pos = mul(corner4.pos, view);
	corner4.pos = mul(corner4.pos, projection);


	output.Append(corner1);
	output.Append(corner2);
	output.Append(corner3);
	output.Append(corner4);
}