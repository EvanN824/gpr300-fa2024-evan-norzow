//We'll be doing Gamma Correction
#version 450

out vec4 FragColor;
in vec2 UV;
uniform sampler2D _ColorBuffer;
uniform float _Gamma;

void main(){
	vec3 color = texture(_ColorBuffer,UV).rgb;
	FragColor = vec4(color,1.0);
	//ganked from learnOpenGL
	FragColor.rgb = pow(FragColor.rgb, vec3(1.0/_Gamma));
}
