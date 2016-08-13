cbuffer ViewProjectionConstantBuffer : register(b0)
{
	matrix view[2];
	matrix projection[2];
};

struct GSInput
{
	float4 pos : SV_POSITION;
	float3 texCoord : TEXCOORD;
};

struct GSOutput
{
	float4 pos : SV_POSITION;
	float3 texCoord : TEXCOORD;
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
			element.texCoord = input[j].texCoord;

			element.pos = input[j].pos;
			element.pos = mul(element.pos, view[i]);
			element.pos = mul(element.pos, projection[i]);

			output.Append(element);
		}
	}
}