uniform sampler2D u_tex;
in vec2 v_texcoord;
in float v_light;
out vec4 o_color;
void main() {
	o_color = texture(u_tex, v_texcoord)*v_light;
	if (o_color.a < .5)
		discard;
}
