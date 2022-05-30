uniform vec2 u_smoothenOffs;

attribute vec4 a_0;
attribute vec2 a_2;

varying vec2 v_uv[2];

void main()
{
	vec2 uv = a_2;
	v_uv[0] = vec2(uv.x, 1.0 - uv.y);
	v_uv[1] = v_uv[0] + u_smoothenOffs;
	gl_Position = a_0;
}
