attribute vec4 a_0;
attribute vec2 a_2;
uniform float u_convergence;
varying vec2 v_uv[5];
void main ()
{
  vec2 uv_1;
  uv_1.y = a_2.y;
  uv_1.x = (floor((
    (256.0 * a_2.x)
   + 12.0)) / 280.0);
  v_uv[0] = (uv_1 + vec2(-0.003571429, 0.0));
  vec2 tmpvar_2;
  tmpvar_2.y = 0.0;
  tmpvar_2.x = (u_convergence / 1120.0);
  v_uv[1] = (uv_1 + tmpvar_2);
  v_uv[2] = uv_1;
  vec2 tmpvar_3;
  tmpvar_3.y = 0.0;
  tmpvar_3.x = (-(u_convergence) / 1120.0);
  v_uv[3] = (uv_1 + tmpvar_3);
  v_uv[4] = (uv_1 + vec2(0.003571429, 0.0));
  gl_Position = a_0;
}

