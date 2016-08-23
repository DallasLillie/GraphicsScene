cbuffer ViewProjectionConstantBuffer : register(b0)
{
	matrix view[2];
	matrix projection[2];
};

cbuffer CamPosBuffer : register(b1)
{
	float4 cameraPosition;
}

struct particle
{
	float3 pos;
	float3 velocity;
	float2 size;
	float age;
	unsigned int type;
};

StructuredBuffer<particle> inputData : register(t0);

struct GSOutput
{
	float4 pos : SV_POSITION;
	float3 wPos : WPOSITION;
	float2 texCoord : TEXCOORD0;
	float3 normal : NORMAL;
	uint viewport : SV_ViewportArrayIndex;
};

[maxvertexcount(6)]
void main(
	point float4 input[1] : SV_POSITION,
	uint index : SV_PrimitiveID,
	inout TriangleStream< GSOutput > output
)
{
	float width = (inputData[index].size*0.5f).x;
	float height = (inputData[index].size*0.5f).y;
	if (inputData[index].age > 0.0f)
	{
		for (unsigned int i = 0; i < 2; ++i)
		{
			GSOutput corner1 = { float4(width, height, 0.0f, 1.0f),		float3(width, height, 0.0f),	float2(0.0f,0.0f),	float3(0, 0, 1.0f),	i };
			GSOutput corner2 = { float4(-width, height, 0.0f, 1.0f),	float3(-width, height, 0.0f),	float2(1.0f,0.0f),	float3(0, 0, 1.0f),	i };
			GSOutput corner3 = { float4(width, -height, 0.0f, 1.0f),	float3(width, -height, 0.0f),	float2(0.0f,1.0f),	float3(0, 0, 1.0f),	i };
			GSOutput corner4 = { float4(-width, -height, 0.0f,1.0f),	float3(-width, -height, 0.0f),	float2(1.0f,1.0f),	float3(0, 0, 1.0f),	i };



			float3 zAxis = cameraPosition.xyz- inputData[index].pos;
			zAxis = normalize(zAxis);
			float3 xAxis = cross(float3(0.0f,1.0f,0.0f),zAxis);
			xAxis = normalize(xAxis);
			float3 yAxis = cross(zAxis, xAxis);
			yAxis = normalize(yAxis);

			float4x4 worldMatrix;
			worldMatrix[0] = float4(xAxis, 0.0f);
			worldMatrix[1] = float4(yAxis, 0.0f);
			worldMatrix[2] = float4(zAxis, 0.0f);
			worldMatrix[3] = float4(inputData[index].pos, 1.0f);
			worldMatrix = transpose(worldMatrix);

			corner1.pos += float4(inputData[index].pos,1.0f);
			corner1.pos = mul(corner1.pos, worldMatrix);
			corner1.pos = mul(corner1.pos, view[i]);
			corner1.pos = mul(corner1.pos, projection[i]);

			corner2.pos += float4(inputData[index].pos, 1.0f);
			corner2.pos = mul(corner2.pos, worldMatrix);
			corner2.pos = mul(corner2.pos, view[i]);
			corner2.pos = mul(corner2.pos, projection[i]);

			corner3.pos += float4(inputData[index].pos, 1.0f);
			corner3.pos = mul(corner3.pos, worldMatrix);
			corner3.pos = mul(corner3.pos, view[i]);
			corner3.pos = mul(corner3.pos, projection[i]);

			corner4.pos += float4(inputData[index].pos, 1.0f);
			corner4.pos = mul(corner4.pos, worldMatrix);
			corner4.pos = mul(corner4.pos, view[i]);
			corner4.pos = mul(corner4.pos, projection[i]);

			output.Append(corner1);
			output.Append(corner2);
			output.Append(corner3);
			output.Append(corner4);
		}
	}
	
}