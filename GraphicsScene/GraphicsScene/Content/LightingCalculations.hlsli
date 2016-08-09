
struct LIGHT
{
	float4 position;
	float4 normal;
	float4 color;
	//OuterConeRatio, InnerConeRatio, LightRadius, Padding
	float4 ratio;
};

float3 CalculateSpecularLighting(LIGHT light,float3 surfacePosition,float3 camPosition,float3 surfaceNormal,float3 lightColor)
{
	float3 toLight = light.position.xyz - surfacePosition;
	if (light.position.w == 0)
	{
		toLight = normalize(-1.0f * light.normal);
	}
	float3 toCamera = camPosition - surfacePosition;
	float3 reflectVector = reflect(normalize(-1.0f*toLight), normalize(surfaceNormal));
	float lightRatio = saturate(dot(light.normal, surfaceNormal));
	float ratio = saturate(dot(normalize(toCamera), normalize(reflectVector)));
	ratio = pow(ratio, 256);
	return ratio*lightColor;
}

float3 CalculateDirectionalLighting(LIGHT directionalLight, float3 surfaceNormal)
{
	float lightRatio = saturate(dot(directionalLight.normal.xyz*-1.0f, surfaceNormal));
	//lightRatio += 0.9;
	//return directionalLight.color.xyz* lightRatio;
	return directionalLight.color.xyz* lightRatio;
}

float3 CalculatePointLighting(LIGHT pointLight, float3 surfacePosition)
{
	//pointLight.normal = float4(normalize(pointLight.position.xyz - surfacePosition), 0);
	//float lightRatio = saturate(dot(pointLight.normal.xyz, surfaceNormal));
	float attenuation = 1.0f - saturate(length(pointLight.position.xyz - surfacePosition) / pointLight.ratio.z);
	//attenuation *= attenuation;
	//attenuation += 0.1;
	//attenuation *= lightRatio;
	return pointLight.color.xyz * attenuation*attenuation;
}

float3 CalculateConeLighting(LIGHT coneLight, float3 surfacePosition, float3 surfaceNormal)
{
	float3 lightDirection = normalize(coneLight.position.xyz - surfacePosition);
	float surfaceRatio = saturate(dot(-1.0f * lightDirection, normalize(coneLight.normal.xyz)));
	float radiusFalloff = 1.0f - saturate(length(coneLight.position.xyz - surfacePosition) / coneLight.ratio.z);
	float attenuation = 1.0f - saturate((coneLight.ratio.y - surfaceRatio) / (coneLight.ratio.y - coneLight.ratio.x));
	attenuation *= radiusFalloff;
	float lightRatio = saturate(dot(lightDirection, surfaceNormal));
	//lightRatio += 0.1;
	//attenuation += 0.1;
	return coneLight.color.xyz * lightRatio * attenuation;
}