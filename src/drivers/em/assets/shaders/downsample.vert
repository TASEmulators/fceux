uniform vec2 u_offsets[8];

attribute vec4 a_0;
attribute vec2 a_2;

varying vec2 v_uv[8];

void main()
{
	gl_Position = a_0;
	for (int i = 0; i < 8; i++) {
		v_uv[i] = a_2 + u_offsets[i];
	}
}
