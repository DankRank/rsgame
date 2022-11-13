uniform mat4 u_viewproj;
in vec3 i_position;
in vec4 i_color;
out vec4 v_color;
void main() {
	gl_Position = u_viewproj * vec4(i_position, 1);
	v_color = i_color;
}
