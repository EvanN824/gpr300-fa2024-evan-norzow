#version 450
out vec4 FragColor; //The color of this fragment

in vec2 UV;

uniform vec3 _EyePos;
uniform mat4 _LightViewProj;
uniform vec3 _LightDirection;
uniform vec3 _LightColor = vec3(1.0); //White light
//Base ambient lighting
//It's half the color of the background to make it blend in!!

//KD + KS should not exceed 1! for realism at least
struct Material{
	float Ka; //ambient coefficient (0-1)
	float Kd; //Diffuse coefficient (0-1)
	float Ks; //Specular coefficient (0-1)
	float Shininess; //Affects size of specular highlight
};
uniform Material _Material;

//deferredLit.frag
struct PointLight
{
	vec3 position;
	float radius;
	vec4 color;
};
#define MAX_POINT_LIGHTS 64
uniform PointLight _PointLights[MAX_POINT_LIGHTS];


uniform layout(location = 0) sampler2D _gPositions;
uniform layout(location = 1) sampler2D _gNormals;
uniform layout(location = 2) sampler2D _gAlbedo;
uniform layout(location = 3) sampler2D _ShadowMap;

vec3 CalcPointLight(PointLight pl, vec3 normal, vec3 wPos)
{
	vec3 diff = pl.position - wPos;
	vec3 toLight = normalize(diff);

	float diffuseFactor = max(dot(normal,toLight),0.0);
	//Direction towards eye
	vec3 toEye = normalize(_EyePos - wPos);
	//Blinn-phong uses half angle
	vec3 h = normalize(toLight + toEye);
	float specularFactor = pow(max(dot(normal,h),0.0),_Material.Shininess);

	vec3 lightColor = (diffuseFactor + specularFactor) * pl.color.xyz;
	//attenuate here.
	float d = length(diff);
	//linear attenuation
	lightColor *= clamp(((pl.radius - diff)/pl.radius),0.0,1.0);
	return lightColor;
}

float ShadowCalc(vec4 fragPosLightSpace, float bias)
{
	//yoinked from LearnOpenGL
	vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;
	projCoords = projCoords * 0.5f + 0.5f;
	//float closestDepth = texture(_ShadowMap, projCoords.xy).r;
	float currentDepth = projCoords.z;

	//return step(closestDepth,currentDepth - bias);
	float totalShadow = 0;
	vec2 texelOffset = 1.0 / textureSize(_ShadowMap,0);
	for (int y = -1; y <=1; y++)
	{
		for (int x = -1; x <=1; x++)
		{
			vec2 uv = projCoords.xy + vec2(x * texelOffset.x, y * texelOffset.y);
			totalShadow += step(texture(_ShadowMap,uv).r, currentDepth - bias);
		}
	}
	totalShadow /= 9.0;

	return totalShadow;
}

void main(){
	//Make sure fragment normal is still length 1 after interpolation.
	//vec3 normal = normalize(fs_in.WorldNormal);

	//Normal mapping goofiness
	vec3 normal = texture(_gNormals, UV).xyz;
	vec3 worldPos = texture(_gPositions, UV).xyz;
	vec3 albedo = texture(_gAlbedo, UV).xyz;

	float bias = max(0.05 * (1.0 - dot(normal, -_LightDirection)), 0.005);


	vec4 LightSpacePos = _LightViewProj * vec4(worldPos, 1.0f);
	float shadow = ShadowCalc(LightSpacePos, bias);

	//Light pointing straight down
	vec3 toLight = -_LightDirection;
	float diffuseFactor = max(dot(normal,toLight),0.0);
	//Direction towards eye
	vec3 toEye = normalize(_EyePos - worldPos);
	//Blinn-phong uses half angle
	vec3 h = normalize(toLight + toEye);
	float specularFactor = pow(max(dot(normal,h),0.0),_Material.Shininess);
	//Combo of specular and diffuse reflection
	vec3 lightColor = (((albedo * _Material.Ka) + (1.0f - shadow)) * (_Material.Kd * diffuseFactor + _Material.Ks * specularFactor)) * _LightColor;

	vec3 totalLight = vec3(0);
	totalLight += lightColor;
	for (int i = 0; i < 2; i++)
	{
		totalLight += CalcPointLight(_PointLights[i], normal, worldPos);
	}

	FragColor = vec4(albedo * totalLight,1.0);
}