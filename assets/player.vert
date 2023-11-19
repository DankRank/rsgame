uniform mat4 u_viewproj;
uniform vec3 u_viewpos;
uniform vec3 u_viewlook;
in vec4 i_position;
in vec2 i_texcoord;
out vec2 v_texcoord;
#define PI 3.1415927410125732421875
#define NROTS 8.
void main() {
	vec3 right = normalize(vec3(-u_viewlook.z, 0, u_viewlook.x));
	mat3 textrans = mat3(1.333*right, vec3(0, -2, 0), vec3(0, .4, 0)-right*.667);
	gl_Position = u_viewproj * vec4(i_position.xyz + textrans*vec3(i_texcoord, 1), 1);
	vec2 d = i_position.xz - u_viewpos.xz;
	float rot = floor(fract((atan(d.x, -d.y) - i_position.w + PI*(NROTS+1.)/(2.*NROTS))/(2.*PI)) * NROTS)/NROTS;
	v_texcoord = i_texcoord/NROTS + vec2(rot, 0);
}
