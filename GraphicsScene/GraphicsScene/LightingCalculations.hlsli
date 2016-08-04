

struct LIGHT
{
	float3 position;
	float3 normal;
	float3 color;
	float coneRatio;
};

float3 CalculateDirectionLighting(LIGHT directionalLight, float3 surfaceNormal)
{
	float lightRatio = saturate(dot(directionalLight.position, surfaceNormal));
	return directionalLight.color* lightRatio;
}

float3 CalculatePointLighting(LIGHT pointLight, float3 surfacePosition, float3 surfaceNormal)
{
	pointLight.normal = normalize(pointLight.position - surfacePosition);
	//float lightRatio = saturate(dot(pointLight.normal, surfaceNormal));
	float attenuation = 1.0f - saturate(length(pointLight.position - surfacePosition)) / (lightRatio.coneRatio*-1.0f);
	attenuation *= attenuation;
	return pointLight.color* attenuation;
}

float3 CalculateConeLighting(LIGHT coneLight, float surfacePosition, float3 surfaceNormal)
{
	float3 lightDirection = normalize(coneLight.position - surfacePosition);
	float surfaceRatio = saturate(dot(-1.0f * lightDirection, coneLight.normal));
	return lightDirection;
}