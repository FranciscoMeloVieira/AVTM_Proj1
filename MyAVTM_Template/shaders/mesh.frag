#version 430

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
	vec3 lightDir;
	vec2 tex_coord;
} DataIn;

uniform Materials mat;

uniform sampler2D texmap;
uniform sampler2D texmap1;
uniform sampler2D texmap2;
uniform sampler2D texmap3;

uniform vec4 pl_loc_1;
uniform vec4 pl_loc_2;
uniform vec4 pl_loc_3;
uniform vec4 pl_loc_4;
uniform vec4 pl_loc_5;
uniform vec4 pl_loc_6;

uniform int texMode;
uniform bool spotlight_mode;
uniform bool pointlight_mode;
uniform vec4 coneDir;
uniform float spotCosCutOff;

out vec4 colorOut;

in vec4 local_pos;
vec4 calculateColor(float intensity, vec4 spec){
		vec4 color = vec4(0.0f, 0.0f, 0.0f, 0.0f);
		vec4 texel, texel1;

		if(texMode == 0) //no texturing
				color = vec4(max(intensity * mat.diffuse + spec, mat.ambient).rgb, 1.0);

		else if(texMode == 1) // modulate diffuse color with texel color
		{
			texel = texture(texmap2, DataIn.tex_coord);  // texel from lighwood.tga
			color = vec4(max(intensity * mat.diffuse * texel + spec,0.07 * texel).rgb, 1.0);
		}
		else if (texMode == 2) // diffuse color is replaced by texel color
		{
			texel = texture(texmap, DataIn.tex_coord);  // texel from stone.tga
			color = vec4(max(intensity*texel + spec, 0.07*texel).rgb, 1.0);
		}
		else if (texMode == 3) // diffuse color is replaced by texel color
		{
			texel = texture(texmap3, DataIn.tex_coord / 2);  // texel from checker.tga
			color = vec4(max(intensity*texel + spec, 0.07*texel).rgb, 1.0);
		}
		else // multitexturing
		{
			texel = texture(texmap2, DataIn.tex_coord);  // texel from lighwood.tga
			texel1 = texture(texmap, DataIn.tex_coord);  // texel from checker.tga
			color = vec4(max(intensity*texel*texel1 + spec, 0.07*texel*texel1).rgb, 1.0);
		}

		return color;
	}

vec4 pointLight(vec4 pl_loc, vec3 n, vec3 e){

		vec4 spec = vec4(0.0);
		float intensity = 0.0f;
		float intSpec = 0.0f;
		vec3 l = local_pos.xyz - pl_loc.xyz;
		float dis = length(l);

		intensity = max(dot(n,l), 0.0);
		if (intensity > 0.0) {

			intensity = max(dot(n,l), 0.0);
			if (intensity > 0.0) {
				vec3 h = normalize(l + e);
				intSpec = max(dot(h,n), 0.0);
				spec = mat.specular * pow(intSpec, mat.shininess);
			}
		}

		float constant = 2.0f;
		float linear = 0.3f;
		float quadratic = 0.9f;

		float att = 1 / ( constant + linear * dis + quadratic * pow(dis,2));

		return calculateColor(intensity,spec)*att;
}

void main() {

	vec4 spec = vec4(0.0);
	float intensity = 0.0f;
	float intSpec = 0.0f;

	float att = 0.0;
	float spotExp = 60.0;

	vec3 n = normalize(DataIn.normal);
	vec3 l = normalize(DataIn.lightDir);
	vec3 e = normalize(DataIn.eye);
	vec3 sd = normalize(coneDir.xyz);

	//Point light locations
	vec4 pl_loc[6] = { pl_loc_1, pl_loc_2, pl_loc_3, pl_loc_4, pl_loc_5, pl_loc_6 };

	if(spotlight_mode == true)  {  //Scene iluminated by a spotlight
		float spotCos = dot(-l, sd);
		if(spotCos > spotCosCutOff)  {	//inside cone?
			att = pow(spotCos, spotExp);
			intensity = max(dot(n,l), 0.0) * att;
			if (intensity > 0.0) {
				vec3 h = normalize(l + e);
				intSpec = max(dot(h,n), 0.0);
				spec = mat.specular * pow(intSpec, mat.shininess) * att;
			}
		}
		colorOut = calculateColor(intensity,spec);
	}

	if(pointlight_mode){
		for ( int i = 0; i < 6 ;i++){
			colorOut += pointLight(pl_loc[i],n,e);
			}
		}

	else{
		colorOut = calculateColor(intensity,spec);
	}
}