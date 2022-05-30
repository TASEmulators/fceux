attribute vec4 a_0;
attribute vec2 a_2;

uniform vec2 u_uvOffset;
uniform float u_vScale;

varying vec2 v_uv;

void main()
{
	gl_Position = a_0;
	v_uv = a_2 + u_uvOffset;
	v_uv = vec2(v_uv.x, u_vScale * (v_uv.y-0.5) + 0.5);
}
