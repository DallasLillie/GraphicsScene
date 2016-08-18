
cbuffer ModelConstantBuffer : register(b0)
{
	matrix model;
};

struct VertexShaderInput
{
	float3 pos : POSITION;
	float3 velocity : VELOCITY;
	float3 normal : NORMAL;
	float2 texCoord : TEXCOORD;
	float2 size : SIZE;
	float age : AGE;
	unsigned int type : TYPE;
};

struct VertexShaderOutput
{
	float4 pos : SV_POSITION;
	float3 velocity : VELOCITY;
	float3 normal : NORMAL;
	float2 texCoord : TEXCOORD;
	float2 size : SIZE;
	float age : AGE;
	unsigned int type : TYPE;
};

// Simple shader to do vertex processing on the GPU.
VertexShaderOutput main(VertexShaderInput input)
{
	VertexShaderOutput output = (VertexShaderOutput)0;
	float4 pos = float4(input.pos, 1.0f);
	float4 normal = float4(input.normal, 0.0f);

	pos = mul(pos, model);
	normal = mul(normal, model);

	//zAxis = camPos - particle.pos;
	//zAxis = normalize(zAxis);
	//float3 up = float3(0.0f, 1.0f, 0.0f);
	//xAxis = cross(up, zAxis);
	//xAxis = normalize(xAxis);
	//yAxis = cross(zAxis, xAxis);
	//yAxis = normalize(yAxis);

	output.pos = pos;
	output.normal = normal;
	output.texCoord = input.texCoord;
	output.velocity = input.velocity;
	output.size = input.size;
	output.age = input.age;
	output.type = input.type;

	return output;
}
