#version 430

#define MAX_LIGHTS 9

struct Light {
    vec4 position;   // eye space; w=1 point, w=0 directional
    vec4 direction;  // eye space, used only if spotlight
    float cosCutOff;
    float spotExp;
    int type;        // 0=directional, 1=point, 2=spot
	bool activated; // This makes so we activation is not dependent on light type, as a consequence it takes more memory but it is more flexible, whatever.
};

uniform Light lights[MAX_LIGHTS];
uniform int lightCount;

struct Materials {
	vec4 diffuse;
	vec4 ambient;
	vec4 specular;
	vec4 emissive;
	float shininess;
	int texCount;
};

in Data {
	vec3 normal;
	vec3 eye;
	vec3 lightDir;// let's remove this when we can
	vec2 tex_coord;
} DataIn;

uniform Materials mat;

uniform sampler2D texmap;
uniform sampler2D texmap1;
uniform sampler2D texmap2;
uniform sampler2D texmap3;
uniform sampler2D texmap4;
uniform sampler2D texmap5;

uniform int texMode;
uniform bool spotlight_mode;
uniform vec4 coneDir;
uniform float spotCosCutOff;

uniform bool transparent;
uniform bool shadowMode;

out vec4 colorOut;

void main() {

    if (shadowMode) {
        colorOut = vec4(0.5, 0.5, 0.5, 1.0);
        return;
    }

	vec3 n = normalize(DataIn.normal);
	vec3 e = normalize(DataIn.eye);
	int lightCount = MAX_LIGHTS;
	vec4 totalDiffuseSpec = vec4(0.0);
	for (int i = 0; i < lightCount; i++) {
        if (!lights[i].activated) continue;

        vec3 l;
        float att = 1.0;

        if (lights[i].type == 0) {
            l = normalize(lights[i].direction.xyz);
        } else {
            vec3 lightDir = lights[i].position.xyz + DataIn.eye; 
            float dist = length(lightDir);
            l = normalize(lightDir);
            att = 1.0 / (1.0 + 0.007 * dist + 0.00008 * dist * dist);

            if (lights[i].type == 2) {
                vec3 sd = normalize(lights[i].direction.xyz);
                float spotCos = dot(-l, sd);
                att *= (spotCos > lights[i].cosCutOff) ? pow(spotCos, lights[i].spotExp) : 0.0;
            }
        }

        float intensity = max(dot(n, l), 0.0) * att;
        vec4 spec = vec4(0.0);
        if (intensity > 0.0) {
            vec3 h = normalize(l + e);
            float intSpec = max(dot(h, n), 0.0);
            spec = mat.specular * pow(intSpec, mat.shininess) * att;
        }
        totalDiffuseSpec += intensity * mat.diffuse + spec;
    }

	vec4 texel = vec4(1.0);
    vec4 texel1 = vec4(1.0);
    vec4 ambientColor = mat.ambient;

    if (texMode == 0) {
        texel = vec4(1.0);
    } else if (texMode == 1) {
        texel = texture(texmap2, DataIn.tex_coord);
    } else if (texMode == 2) {
        texel = texture(texmap, DataIn.tex_coord);
    } else if (texMode == 3) {
        texel = texture(texmap3, DataIn.tex_coord / 2.0);
    } else if (texMode == 4) {
        texel = texture(texmap4, DataIn.tex_coord / 2.0);
    } else if (texMode == 5) {
        texel = texture(texmap5, DataIn.tex_coord / 2.0);
    } else {
        texel = texture(texmap2, DataIn.tex_coord);
        texel1 = texture(texmap3, DataIn.tex_coord);
        texel = mix(texel, texel1, 0.5);
        texel.a = 0.80; // Change this to reduce reflection
    }

    vec3 rgbColor = (totalDiffuseSpec * texel + ambientColor * texel + mat.emissive).rgb;
    float finalAlpha = texel.a * mat.diffuse.a;

    if ((finalAlpha == 1.0 && transparent) || (finalAlpha != 1.0 && !transparent) && texMode <= 6) // If floor is called then no discard is needed, it's always called when needed
    {
        discard;
    }

	float dist = length(DataIn.eye);
    float fogAmount = clamp((300.0 - dist) / (300.0 - 50.0), 0.0, 1.0);
    vec3 foggedRGB = mix(vec3(0.4784, 0.6863, 0.7765), rgbColor, fogAmount);
    colorOut = vec4(foggedRGB, finalAlpha);
}