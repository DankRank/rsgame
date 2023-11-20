uniform mat4 u_viewproj;
uniform mat3 u_textrans;
uniform vec3 u_viewpos;
in vec4 i_position;
in vec2 i_texcoord;
out vec2 v_texcoord;
#define PI 3.1415927410125732421875
#define NROTS 8.
void main() {
	gl_Position = u_viewproj * vec4(i_position.xyz + u_textrans*vec3(i_texcoord, 1), 1);
	/* Sprite index calculation
	 * There are three rotations we need to take into account:
	 * - player yaw (stored in i_position.w)
	 * - look yaw (derived via d, ray pointing from player to camera)
	 * - rotation of the sprites as the index increases.
	 * The convention for yaw is 0 points to -Z, rotation is counter-clockwise,
	 * i.e from -Z to -X. So to compute look yaw we need atan(-x/-z).
	 * The convention for sprites is taken from Doom: 0 looks at you,
	 * increasing indices rotate clockwise.
	 * Look yaw is essentially the yaw, at which the player would be looking
	 * into the camera. To compute the sprite angle we take the difference
	 * between the two yaws, and negate it to fix the clockwiseness.
	 */
	vec2 d = u_viewpos.xz - i_position.xz;
	float rot = atan(-d.x, -d.y) - i_position.w;
	// Add 1/16th of a rotation, to offset the transition between sprites.
	rot += 2.*PI/(2.*NROTS);
	// normalize to 0..1 range
	rot = fract(rot/(2.*PI));
	// discretize and add to texture coords
	rot = floor(rot * NROTS);
	v_texcoord = vec2(i_texcoord.x, (i_texcoord.y + rot)/NROTS);
}
