uniform vec2 u_offsets[8];
attribute vec4 a_0;
attribute vec2 a_2;
varying vec2 v_uv[8];
void main ()
{
  gl_Position = a_0;
  v_uv[0] = (a_2 + u_offsets[0]);
  v_uv[1] = (a_2 + u_offsets[1]);
  v_uv[2] = (a_2 + u_offsets[2]);
  v_uv[3] = (a_2 + u_offsets[3]);
  v_uv[4] = (a_2 + u_offsets[4]);
  v_uv[5] = (a_2 + u_offsets[5]);
  v_uv[6] = (a_2 + u_offsets[6]);
  v_uv[7] = (a_2 + u_offsets[7]);
}

