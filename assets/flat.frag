in vec2 v_texcoord;
in float v_light;
out vec4 o_color;
void main() {
	o_color = vec4(vec3(v_light), 1);
}
