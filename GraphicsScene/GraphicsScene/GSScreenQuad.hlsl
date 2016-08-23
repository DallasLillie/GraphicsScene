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
	float2 texCoords : TEXCOORD;
};

[maxvertexcount(4)]
void main(
	point GSInput input[1],
	inout TriangleStream< GSOutput > output
)
{
	GSOutput corner1 = { float4(-1.0f,  1.0f,  0.1f,1.0f),float2(0.0f,  0.0f)};
	GSOutput corner2 = { float4(1.0f,  1.0f, 0.1f,1.0f),float2(1.0f,  0.0f)};
	GSOutput corner3 = { float4(-1.0f,  -1.0f, 0.1f,1.0f),float2(0.0f,  1.0f)};
	GSOutput corner4 = { float4(1.0f,  -1.0f, 0.1f,1.0f),float2(1.0f,  1.0f)};

	output.Append(corner1);
	output.Append(corner2);
	output.Append(corner3);
	output.Append(corner4);

}