attribute vec4 a_0;
attribute vec2 a_2;
uniform vec2 u_uvOffset;
uniform float u_vScale;
varying vec2 v_uv;
void main ()
{
  gl_Position = a_0;
  v_uv = (a_2 + u_uvOffset);
  vec2 tmpvar_1;
  tmpvar_1.x = v_uv.x;
  tmpvar_1.y = ((u_vScale * (v_uv.y - 0.5)) + 0.5);
  v_uv = tmpvar_1;
}

