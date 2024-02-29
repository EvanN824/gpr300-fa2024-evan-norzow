#version 450
out vec4 FragColor; //The color of this fragment
in Surface{
	vec3 WorldPos;
	vec3 WorldNormal;
	vec2 TexCoord;
	mat3 TBN;
}fs_in;

//geometryPass.frag 
layout(location = 0) out vec3 gPosition; //Worldspace position
layout(location = 1) out vec3 gNormal; //Worldspace normal 
layout(location = 2) out vec3 gAlbedo;

uniform sampler2D _MainTex;
uniform sampler2D _NormalMap;

void main(){
	gPosition = fs_in.WorldPos;
	gAlbedo = texture(_MainTex,fs_in.TexCoord).rgb;
	
	vec3 normal = texture(_NormalMap, fs_in.TexCoord).rgb;
	normal = normal * 2.0 - 1.0;
	gNormal = normalize(fs_in.WorldNormal);
}

