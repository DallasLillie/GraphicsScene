cbuffer ViewProjectionConstantBuffer : register(b0)
{
	matrix view[2];
	matrix projection[2];
};

struct GSInput
{
	float4 pos : SV_POSITION;
	float3 velocity : VELOCITY;
	float3 normal : NORMAL;
	float2 texCoord : TEXCOORD;
	float2 size : SIZE;
	float age : AGE;
	unsigned int type : TYPE;
};

struct GSOutput
{
	float4 pos : SV_POSITION;
	float3 wPos : WPOSITION;
	float2 texCoord : TEXCOORD0;
	float3 normal : NORMAL;
	uint viewport : SV_ViewportArrayIndex;
};


[maxvertexcount(8)]
void main(
	point GSInput input[1],
	inout TriangleStream< GSOutput > output
)
{
	for (unsigned int j = 0; j < 1; ++j)
	{
		float width = (input[j].size*0.5f).x;
		float height = (input[j].size*0.5f).y;
		if (input[j].age > 0.0f)
		{
			for (unsigned int i = 0; i < 2; ++i)
			{
				GSOutput corner1 = { float4(-width, height, 0.0f, 1.0f),	float3(-width, height, 0.0f),	float2(0.0f,0.0f),	input[j].normal,i };
				GSOutput corner2 = { float4(width, height, 0.0f, 1.0f),		float3(width, height, 0.0f),	float2(1.0f,0.0f),	input[j].normal,i };
				GSOutput corner3 = { float4(-width, -height, 0.0f,1.0f),	float3(-width, -height, 0.0f),	float2(0.0f,1.0f),	input[j].normal,i };
				GSOutput corner4 = { float4(width, -height, 0.0f, 1.0f),	float3(width, -height, 0.0f),	float2(1.0f,1.0f),	input[j].normal,i };


				corner1.pos += input[0].pos;
				corner1.pos = mul(corner1.pos, view[i]);
				corner1.pos = mul(corner1.pos, projection[i]);

				corner2.pos += input[0].pos;
				corner2.pos = mul(corner2.pos, view[i]);
				corner2.pos = mul(corner2.pos, projection[i]);

				corner3.pos += input[0].pos;
				corner3.pos = mul(corner3.pos, view[i]);
				corner3.pos = mul(corner3.pos, projection[i]);

				corner4.pos += input[0].pos;
				corner4.pos = mul(corner4.pos, view[i]);
				corner4.pos = mul(corner4.pos, projection[i]);

				output.Append(corner1);
				output.Append(corner2);
				output.Append(corner3);
				output.Append(corner4);
			}
		}
	}
}