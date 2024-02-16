#version 450
out vec4 FragColor; //The color of this fragment
in Surface{
	vec3 WorldPos;
	vec3 WorldNormal;
	vec2 TexCoord;
	mat3 TBN;
}fs_in;

in vec4 LightSpacePos;

uniform sampler2D _MainTex; //2D texture sampler
uniform sampler2D _NormalMap;
uniform sampler2D _ShadowMap;

uniform vec3 _EyePos;
//Light pointing straight down
uniform vec3 _LightDirection;
uniform vec3 _LightColor = vec3(1.0); //White light
//Base ambient lighting
//It's half the color of the background to make it blend in!!
uniform vec3 _AmbientColor = vec3(0.3,0.4,0.46);

//KD + KS should not exceed 1! for realism at least
struct Material{
	float Ka; //ambient coefficient (0-1)
	float Kd; //Diffuse coefficient (0-1)
	float Ks; //Specular coefficient (0-1)
	float Shininess; //Affects size of specular highlight
};
uniform Material _Material;

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
	vec3 normal = texture(_NormalMap, fs_in.TexCoord).rgb;
	normal = normal * 2.0 - 1.0;
	normal = normalize(fs_in.TBN * normal);

	float bias = max(0.05 * (1.0 - dot(normal, -_LightDirection)), 0.005);

	float shadow = ShadowCalc(LightSpacePos, bias);

	//Light pointing straight down
	vec3 toLight = -_LightDirection;
	float diffuseFactor = max(dot(normal,toLight),0.0);
	//Direction towards eye
	vec3 toEye = normalize(_EyePos - fs_in.WorldPos);
	//Blinn-phong uses half angle
	vec3 h = normalize(toLight + toEye);
	float specularFactor = pow(max(dot(normal,h),0.0),_Material.Shininess);
	//Combo of specular and diffuse reflection
	vec3 lightColor = (((_AmbientColor * _Material.Ka) + (1.0f - shadow)) * (_Material.Kd * diffuseFactor + _Material.Ks * specularFactor)) * _LightColor;

	vec3 objectColor = texture(_MainTex,fs_in.TexCoord).rgb;
	FragColor = vec4(objectColor * lightColor,1.0);
}
