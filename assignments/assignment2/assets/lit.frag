#version 450
out vec4 FragColor; //The color of this fragment
in Surface{
	vec3 WorldPos;
	vec3 WorldNormal;
	vec2 TexCoord;
	mat3 TBN;
}fs_in;

uniform sampler2D _MainTex; //2D texture sampler
uniform sampler2D _NormalMap;

uniform vec3 _EyePos;
//Light pointing straight down
uniform vec3 _LightDirection = vec3(0.0,-1.0,0.0);
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

void main(){
	//Make sure fragment normal is still length 1 after interpolation.
	//vec3 normal = normalize(fs_in.WorldNormal);

	//Normal mapping goofiness
	vec3 normal = texture(_NormalMap, fs_in.TexCoord).rgb;
	normal = normal * 2.0 - 1.0;
	normal = normalize(fs_in.TBN * normal);

	//Light pointing straight down
	vec3 toLight = -_LightDirection;
	float diffuseFactor = max(dot(normal,toLight),0.0);
	//Direction towards eye
	vec3 toEye = normalize(_EyePos - fs_in.WorldPos);
	//Blinn-phong uses half angle
	vec3 h = normalize(toLight + toEye);
	float specularFactor = pow(max(dot(normal,h),0.0),_Material.Shininess);
	//Combo of specular and diffuse reflection
	vec3 lightColor = (_Material.Kd * diffuseFactor + _Material.Ks * specularFactor) * _LightColor;
	//Ambient lighting
	lightColor+=_AmbientColor * _Material.Ka;
	vec3 objectColor = texture(_MainTex,fs_in.TexCoord).rgb;
	FragColor = vec4(objectColor * lightColor,1.0);
}
