uniform mat4 u_viewproj;
in vec3 i_position;
in float i_light;
out float v_light;
void main() {
	gl_Position = u_viewproj * vec4(i_position, 1);
	v_light = i_light;
}
