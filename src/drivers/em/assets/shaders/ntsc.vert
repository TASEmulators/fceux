attribute vec4 a_0;
attribute vec2 a_2;

uniform vec2 u_noiseRnd;

varying vec2 v_uv[int(NUM_TAPS)];
varying vec2 v_deemp_uv;
varying vec2 v_noiseUV;

#define UV_OUT(i_, o_) v_uv[i_] = vec2(uv.x + (o_)/IDX_W, uv.y)

void main()
{
	vec2 uv = a_2;
	UV_OUT(0,-2.0);
	UV_OUT(1,-1.0);
	UV_OUT(2, 0.0);
	UV_OUT(3, 1.0);
	UV_OUT(4, 2.0);
	v_deemp_uv = vec2(uv.y, 0.0);
	v_noiseUV = vec2(IDX_W/NOISE_W, IDX_H/NOISE_H)*a_2 + u_noiseRnd;
	gl_Position = a_0;
}
