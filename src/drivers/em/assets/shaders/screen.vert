attribute vec4 a_0;
attribute vec3 a_1;
attribute vec2 a_2;

uniform mat4 u_mvp;
uniform vec2 u_uvScale;

varying vec2 v_uv;
varying vec3 v_norm;
varying vec3 v_pos;
varying vec2 v_noiseUV;

void main()
{
	v_uv = 0.5 + u_uvScale * (a_2 - 0.5);
	v_norm = a_1;
	v_pos = a_0.xyz;
// TODO: tsone: duplicated (screen, tv), manual homogenization
	gl_Position = u_mvp * a_0;
	gl_Position = vec4(gl_Position.xy / gl_Position.w, 0.0, 1.0);
	vec2 vwc = gl_Position.xy * 0.5 + 0.5;
	v_noiseUV = vec2(SCREEN_W, SCREEN_H) * vwc / vec2(NOISE_W, NOISE_H);
}
