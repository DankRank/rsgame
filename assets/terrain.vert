uniform mat4 u_viewproj;
in vec3 i_position;
in vec2 i_texcoord;
in float i_light;
out vec2 v_texcoord;
out float v_light;
void main() {
	gl_Position = u_viewproj * vec4(i_position, 1);
	v_texcoord = i_texcoord;
	v_light = i_light;
}
