#version 450

uniform sampler2D texSampler;
uniform vec3 camPos;
uniform vec3 lightPos;
uniform float brightness = 500000;

uniform vec3 baseColor = vec3(.16);
uniform float metallic = 0.0;
uniform float subsurface = 0.0;
uniform float specular = 0.0;
uniform float roughness = .9;
uniform float specularTint = 0.0;
uniform float anisotropic = 0.0;
uniform float sheen = 0.0;
uniform float sheenTint = 0.5;
uniform float clearcoat = 0.0;
uniform float clearcoatGloss = 1.0;

in vec3 worldSpacePos;
in vec3 worldSpaceNormal;
in vec3 worldSpaceTangent;
in vec3 worldSpaceBitangent;
in vec2 UV;

out vec4 outColor;

const float PI = 3.14159265358979323846;

mat3 LocalToWorld, WorldToLocal;

float l1Norm(vec3 v) {
	return abs(v.x) + abs(v.y) + abs(v.z);
}

float angle(vec3 v1, vec3 v2) {
	return acos(dot(v1, v2) / (l1Norm(v1) * l1Norm(v2)));
}

float sqr(float x) {
	return x*x;
}

float SchlickFresnel(float angle) {
	float u = cos(angle);
	float m = clamp(1-u, 0, 1);
	return m*m*m*m*m; // m^5
}

float GTR1(float cos_hn, float a) {
	if (a>=1.0) return 1/PI;
	float a2 = sqr(a);
	float t = 1 + (a2 - 1) * sqr(cos_hn);
	return (a2 - 1) / (PI * log(a2) * t);
}

float GTR2(float cos_hn, float a) {
	float a2 = sqr(a);
	float t = 1 + (a2 - 1) * sqr(cos_hn);
	return a2 / (PI * t * t);
}

float GTR2_aniso(float cos_hn, float cos_hx, float cos_hy, float ax, float ay) {
	return 1.0 / (PI * ax*ay * sqr(sqr(cos_hx/ax) + sqr(cos_hy/ay) + sqr(cos_hn)));
}

float smithG_GGX(float u, float alphaG) {
	float a = sqr(alphaG);
	float b = sqr(u);
	return 1.0 / (u + sqrt(a + b - a*b));
}

float smithG_GGX_aniso(float u, float ux, float uy, float ax, float ay) {
	return 1.0 / (u + sqrt(sqr(ux/ax) + sqr(uy/ay) + sqr(u)));
}

vec3 mon2lin(vec3 v) {
	return vec3(pow(v.x, 2.2), pow(v.y, 2.2), pow(v.z, 2.2));
}

vec3 BRDF(vec3 L, vec3 V, vec3 N, vec3 X, vec3 Y) {
	vec3 baseColor_tex = baseColor * texture(texSampler, UV).rgb;
	float cos_ln = cos(angle(L, N));
	float cos_vn = cos(angle(V, N));

	vec3 H = normalize(L + V);
	float cos_lh = cos(angle(L, H));
	float cos_hn = cos(angle(H, N));
	float cos_hx = cos(angle(H, X));
	float cos_hy = cos(angle(H, Y));
	float cos_lx = cos(angle(L, X));
	float cos_ly = cos(angle(L, Y));
	float cos_vx = cos(angle(V, X));
	float cos_vy = cos(angle(V, Y));

	vec3 C_dlin = mon2lin(baseColor_tex);
	float C_dlum = 0.3*C_dlin.r + .6*C_dlin.g + .1*C_dlin.b; // luminance approximation
	vec3 C_tint = C_dlum > 0 ? C_dlin/C_dlum : vec3(1.0);
	vec3 C_spec0 = mix(specular*.08*mix(vec3(1.0), C_tint, specularTint), C_dlin, metallic);
	vec3 C_sheen = mix(vec3(1.0), C_tint, sheenTint);

	// Diffuse Fresnel
	float F_l = SchlickFresnel(cos_ln);
	float F_v = SchlickFresnel(cos_vn);
	float F_d90 = 0.5 + (2 * cos_lh*cos_lh * roughness);
	float F_d = mix(1.0, F_d90, F_l) * mix(1.0, F_d90, F_v);

	// Subsurface
	float F_ss90 = cos_lh*cos_lh*roughness;
	float F_ss = mix(1.0, F_ss90, F_l) * mix(1.0, F_ss90, F_v);
	float ss = 1.25 * (F_ss * (1.0 / (cos_ln + cos_vn) - 0.5) + 0.5);

	// Specular
	float aspect = sqrt(1-anisotropic*0.9);
	float ax = max(0.001, (roughness*roughness)/aspect);
	float ay = max(0.001, (roughness*roughness)*aspect);
	float D_s = GTR2_aniso(cos_hn, cos_hx, cos_hy, ax, ay);
	float F_h = SchlickFresnel(cos_lh);
	vec3 F_s = mix(C_spec0, vec3(1.0), F_h);
	float G_s = smithG_GGX_aniso(cos_ln, cos_lx, cos_ly, ax, ay) * smithG_GGX_aniso(cos_vn, cos_vx, cos_vy, ax, ay);

	// Sheen
	vec3 F_sheen = F_h * sheen * C_sheen;

	// Clearcoat
	float D_r = GTR1(cos_hn, mix(0.1, .001, clearcoatGloss));
	float F_r = mix(.04, 1.0, F_h);
	float G_r = smithG_GGX(cos_ln, 0.25) * smithG_GGX(cos_vn, 0.25);

	return ((1.0/PI) * mix(F_d, ss, subsurface) * C_dlin + F_sheen) * (1.0-metallic) + G_s*F_s*D_s + 0.25*clearcoat*G_r*F_r*D_r;
}

void main() {

	vec3 viewDir = normalize(camPos - worldSpacePos);
	vec3 lightDir = normalize(lightPos - worldSpacePos);
	vec3 normal = normalize(worldSpaceNormal);
	vec3 b = max(BRDF(lightDir, viewDir, normal, worldSpaceTangent, worldSpaceBitangent), vec3(0.0));
	b *= dot(lightDir, normal);
	float falloff = sqr(lightPos.x - worldSpacePos.x) + sqr(lightPos.y - worldSpacePos.y) + sqr(lightPos.z - worldSpacePos.z);
	b *= (1.0 / falloff);
	b *= brightness;
	outColor = vec4(clamp(b, vec3(0.0), vec3(1.0)), 1.0);
}