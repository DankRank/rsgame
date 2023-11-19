uniform sampler2D u_tex;
in vec2 v_texcoord;
out vec4 o_color;
void main() {
	o_color = texture(u_tex, v_texcoord);
	if (o_color.a < .5)
		discard;
}
