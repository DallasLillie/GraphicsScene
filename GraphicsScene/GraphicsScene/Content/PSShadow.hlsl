
// Per-pixel color data passed through the pixel shader.
struct PixelShaderInput
{
	//TODO: specific shadowmap pixel input.
	float4 pos : SV_POSITION;
	float3 wPos : WPOSITION;
	float2 texCoord : TEXCOORD0;
	float4 tangent : TANGENT;
	float4 biTangent :BTANGENT;
	float3 normal : NORMAL;
	float4 projTex : TEXCOORD1;
};

// A pass-through function for the (interpolated) color data.
float4 main(PixelShaderInput input) : SV_TARGET
{
	float depth = input.projTex.z /= input.projTex.w;

	return float4(depth,depth,depth,depth);
}
