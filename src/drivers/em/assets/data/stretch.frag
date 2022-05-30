uniform highp float u_scanlines;
uniform sampler2D u_sharpenTex;
varying vec2 v_uv[2];
void main ()
{
  lowp vec4 tmpvar_1;
  tmpvar_1 = texture2D (u_sharpenTex, v_uv[0]);
  lowp vec4 tmpvar_2;
  tmpvar_2 = texture2D (u_sharpenTex, v_uv[1]);
  lowp vec3 tmpvar_3;
  tmpvar_3 = ((tmpvar_1.xyz * tmpvar_1.xyz) + (tmpvar_2.xyz * tmpvar_2.xyz));
  lowp vec4 tmpvar_4;
  tmpvar_4.w = 1.0;
  tmpvar_4.xyz = sqrt(mix ((0.5 * tmpvar_3), max (
    (tmpvar_3 - 1.0)
  , 0.0), (u_scanlines * 
    (1.0 - abs(sin((
      (753.9822 * v_uv[0].y)
     - 0.3926991))))
  )));
  gl_FragColor = tmpvar_4;
}

