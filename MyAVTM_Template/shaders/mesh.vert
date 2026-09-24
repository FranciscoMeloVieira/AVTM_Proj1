#version 430

uniform mat4 m_pvm;
uniform mat4 m_viewModel;
uniform mat3 m_normal;


uniform vec4 l_pos;

in vec4 position;
in vec4 normal;    //por causa do gerador de geometria
in vec4 texCoord;
layout (location = 3) in vec3 instancePos;
layout (location = 4) in vec3 instanceSize;

out Data {
	vec3 normal;
	vec3 eye;
	vec3 lightDir;
	vec2 tex_coord;
} DataOut;

void main()
{
    vec3 scaledPosition = position.xyz * instanceSize;

    vec4 instancePosition =
        vec4(instancePos + scaledPosition, 1.0);

    vec4 pos = m_viewModel * instancePosition;

    DataOut.normal = normalize(m_normal * normal.xyz);
    DataOut.lightDir = vec3(l_pos - pos);
    DataOut.eye = vec3(-pos);
    DataOut.tex_coord = texCoord.st;

    gl_Position = m_pvm * instancePosition;
}