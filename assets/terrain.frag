uniform sampler2D u_tex;
in vec2 v_texcoord;
in float v_light;
out vec4 o_color;
void main() {
	if (v_light < 0)
		o_color = texture(u_tex, v_texcoord)*vec4(
			max(-v_light * .6 + .4, 0),
			max(v_light*v_light * .7 - .5, 0),
			0, 1);
	else
		o_color = texture(u_tex, v_texcoord)*vec4(vec3(v_light), 1);
	if (o_color.a < .5)
		discard;
}
