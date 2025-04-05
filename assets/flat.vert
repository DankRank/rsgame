uniform mat4 u_viewproj;
attribute vec3 i_position;
attribute vec4 i_color;
varying vec4 v_color;
void main() {
	gl_Position = u_viewproj * vec4(i_position, 1);
	v_color = i_color;
}
