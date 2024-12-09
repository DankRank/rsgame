uniform sampler2D u_tex;
uniform sampler2D u_lighttex;
in vec2 v_texcoord;
in float v_light;
in float v_aolight;
out vec4 o_color;
void main() {
	o_color = texture(u_tex, v_texcoord)*texture(u_lighttex, vec2(v_light, 0.f))*vec4(vec3(v_aolight), 1.f);
	if (o_color.a < .5)
		discard;
}
