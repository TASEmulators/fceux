attribute vec4 a_0;
attribute vec2 a_2;
uniform vec2 u_noiseRnd;
varying vec2 v_uv[5];
varying vec2 v_deemp_uv;
varying vec2 v_noiseUV;
void main ()
{
  vec2 tmpvar_1;
  tmpvar_1.x = (a_2.x + -0.007142857);
  tmpvar_1.y = a_2.y;
  v_uv[0] = tmpvar_1;
  vec2 tmpvar_2;
  tmpvar_2.x = (a_2.x + -0.003571429);
  tmpvar_2.y = a_2.y;
  v_uv[1] = tmpvar_2;
  v_uv[2] = a_2;
  vec2 tmpvar_3;
  tmpvar_3.x = (a_2.x + 0.003571429);
  tmpvar_3.y = a_2.y;
  v_uv[3] = tmpvar_3;
  vec2 tmpvar_4;
  tmpvar_4.x = (a_2.x + 0.007142857);
  tmpvar_4.y = a_2.y;
  v_uv[4] = tmpvar_4;
  vec2 tmpvar_5;
  tmpvar_5.y = 0.0;
  tmpvar_5.x = a_2.y;
  v_deemp_uv = tmpvar_5;
  v_noiseUV = ((vec2(1.09375, 0.9375) * a_2) + u_noiseRnd);
  gl_Position = a_0;
}

