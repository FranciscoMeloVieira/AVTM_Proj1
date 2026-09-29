#version 430

uniform mat4 m_pvm;
uniform mat4 m_viewModel;
uniform mat3 m_normal;
uniform mat4 m_model;

uniform vec4 l_pos;
uniform int texMode;
uniform vec4 coneDir;

in vec4 position;
in vec4 normal;    //por causa do gerador de geometria
in vec4 texCoord;
in vec4 tangent;
layout (location = 3) in vec3 instancePos;

out Data {
	vec3 normal;
	vec3 eye;
	vec3 lightDir;
	vec2 tex_coord;
} DataOut;

void main () {
	vec4 worldPos = m_model * position + vec4(instancePos, 0.0);
	vec4 pos = m_viewModel * worldPos;

	DataOut.normal = normalize(m_normal * mat3(m_model) * normal.xyz);
	vec3 n = DataOut.normal;
	DataOut.lightDir = vec3(l_pos - pos);
	DataOut.eye = vec3(-pos);
	DataOut.tex_coord = texCoord.st;

	if(texMode == 9) {  //convert eye, light, and cone direction vectors to tangent space

			//Calculate components of TBN basis in eye space
			vec3 t = normalize(m_normal * tangent.xyz);  
			vec3 b = tangent.w * cross(n,t);
			vec3 aux;

			aux.x = dot(t, DataOut.lightDir);
			aux.y = dot(b, DataOut.lightDir);
			aux.z = dot(n, DataOut.lightDir);
			DataOut.lightDir = normalize(aux);

			aux.x = dot(t, DataOut.eye);
			aux.y = dot(b, DataOut.eye);
			aux.z = dot(n, DataOut.eye);
			DataOut.eye = normalize(aux);
		}

	gl_Position = m_pvm * worldPos;	
}