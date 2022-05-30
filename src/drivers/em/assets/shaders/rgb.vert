attribute vec4 a_0;
attribute vec2 a_2;

varying vec2 v_uv;
varying vec2 v_deemp_uv;

void main()
{
	v_uv = a_2;
	v_deemp_uv = vec2(v_uv.y, 0.0);
	gl_Position = a_0;
}
