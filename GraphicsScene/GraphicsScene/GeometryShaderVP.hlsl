cbuffer ViewProjectionConstantBuffer : register(b0)
{
	matrix view[2];
	matrix projection[2];
};

struct GSInput
{
	float4 pos : SV_POSITION;
	//float3 wPos : WPOSITION; //Output from geometry shader
	float2 texCoord : TEXCOORD;
	float4 tangent : TANGENT;
	float4 biTangent :BTANGENT;
	float3 normal : NORMAL;
};

struct GSOutput
{
	float4 pos : SV_POSITION;
	float3 wPos : WPOSITION; //Output from geometry shader
	float2 texCoord : TEXCOORD;
	float4 tangent : TANGENT;
	float4 biTangent :BTANGENT;
	float3 normal : NORMAL;
	uint viewport : SV_ViewportArrayIndex;
};

[maxvertexcount(6)]
void main(
	triangle GSInput input[3],
	inout TriangleStream< GSOutput > output
)
{
	for (uint i = 0; i < 2; ++i)
	{
		GSOutput element;
		element.viewport = i;
		for (int j = 2; j >= 0; --j)
		{
			element.pos = input[j].pos;
			element.wPos = input[j].pos;
			element.texCoord = input[j].texCoord;
			element.tangent = input[j].tangent;
			element.biTangent = input[j].biTangent;
			element.normal = input[j].normal;

			element.pos = mul(element.pos, view[i]);
			element.pos = mul(element.pos, projection[i]);

			output.Append(element);
		}
	}
}