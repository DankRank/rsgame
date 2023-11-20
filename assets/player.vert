uniform mat4 u_viewproj;
uniform vec3 u_viewpos;
uniform vec3 u_viewlook;
in vec4 i_position;
in vec2 i_texcoord;
out vec2 v_texcoord;
#define PI 3.1415927410125732421875
#define NROTS 8.
void main() {
	/* Vertex position is player position + transformed texture position.
	 * The texture coordinates on the input are in range 0..1. We transform
	 * that to be a 4/3 wide, 2 high quad in world space. The origin point also
	 * has to be offset 2/3 left, and 0.4 up (the latter is due to eye height
	 * being 1.6). The coordinates are vertically flipped because we store
	 * textures top-to-bottom. This yields the following affine transform:
	 *                            [ 4/3   0  -2/3 ]
	 * textrans = [ right  up ] x [  0   -2   0.4 ]
	 * Where right and up are vectors. Up is just world up, right is
	 * perpendicular to the camera to achieve that retro billboard look.
	 */
	vec3 right = normalize(vec3(-u_viewlook.z, 0, u_viewlook.x));
	mat3 textrans = mat3(1.333*right, vec3(0, -2, 0), vec3(0, .4, 0)-right*.667);
	gl_Position = u_viewproj * vec4(i_position.xyz + textrans*vec3(i_texcoord, 1), 1);
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
