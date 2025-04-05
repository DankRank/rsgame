uniform sampler2D u_tex;
uniform sampler2D u_lighttex;
varying vec2 v_texcoord;
varying float v_light;
varying float v_aolight;
void main() {
	gl_FragColor = texture2D(u_tex, v_texcoord)*texture2D(u_lighttex, vec2(v_light, 0.))*vec4(vec3(v_aolight), 1.);
	if (gl_FragColor.a < .5)
		discard;
}
