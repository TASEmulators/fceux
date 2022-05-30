attribute vec4 a_0;
attribute vec2 a_2;

uniform float u_convergence;

varying vec2 v_uv[5];

#define TAP(i_, o_) v_uv[i_] = uv + vec2((o_) / RGB_W, 0.0)

void main()
{
	vec2 uv = a_2;
	uv.x = floor(INPUT_W*uv.x + OVERSCAN_W) / IDX_W;
	TAP(0,-4.0);
	TAP(1, u_convergence);
	TAP(2, 0.0);
	TAP(3,-u_convergence);
	TAP(4, 4.0);
	gl_Position = a_0;
}
