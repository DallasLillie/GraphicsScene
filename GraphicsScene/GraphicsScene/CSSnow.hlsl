
//TODO: Header file
struct particle
{
	float3 pos;
	float3 velocity;
	float2 size;
	float age;
	unsigned int type;
};

StructuredBuffer<particle> input;
RWStructuredBuffer<particle> output;

groupshared particle particles[800 * 1 * 1];

float wang_hash(uint seed)
{
	seed = (seed ^ 61) ^ (seed >> 16);
	seed *= 9;
	seed = seed ^ (seed >> 4);
	seed *= 0x27d4eb2d;
	seed = seed ^ (seed >> 15);
	float f0 = float(seed) * (1.0 / 4294967296.0);
	return f0;
}

[numthreads(800, 1, 1)]
void main( uint3 DTid : SV_DispatchThreadID )
{
	uint rSeed = DTid.x;
	float rand = wang_hash(rSeed);

	GroupMemoryBarrierWithGroupSync();
	output[DTid.x*DTid.y*DTid.z] = input[DTid.x*DTid.y*DTid.z];
}