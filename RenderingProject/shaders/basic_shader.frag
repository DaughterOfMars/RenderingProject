#version 450

uniform sampler2D texSampler;
uniform vec3 camPos;
uniform vec3 lightPos;

in vec3 worldPos;
in vec3 worldNormal;
in vec2 UV;

out vec4 outColor;

vec3 ambientColor = vec3(0.1, 0.1, 0.1);
vec3 specColor = vec3(1.0, 1.0, 1.0);
float shininess = 100.0;

vec4 blinnphong(){
    vec3 P = worldPos;
    vec3 N = worldNormal;
    vec3 L = lightPos;
    vec3 lightDir = normalize(L - P);
    float lamb = max(dot(lightDir, N),0.0);
    float spec = 0.0;
    if(lamb > 0.0){
        vec3 viewDir = normalize(camPos-P);
        vec3 halfway = normalize(lightDir + viewDir);
        spec = pow(max(dot(halfway, N), 0.0), shininess);
    }
	vec3 diffuseColor = texture(texSampler, UV).rgb;
    return vec4(ambientColor+lamb*2.0*diffuseColor+spec*specColor, 1.0);
}

void main() {
	outColor = blinnphong();
}