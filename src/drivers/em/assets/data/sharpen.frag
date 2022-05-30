uniform sampler2D u_rgbTex;
uniform vec3 u_sharpenKernel[5];
varying vec2 v_uv[5];
void main ()
{
  lowp vec3 color_1;
  lowp vec4 tmpvar_2;
  tmpvar_2 = texture2D (u_rgbTex, v_uv[0]);
  color_1 = ((u_sharpenKernel[0] * tmpvar_2.xyz) * tmpvar_2.xyz);
  lowp vec4 tmpvar_3;
  tmpvar_3 = texture2D (u_rgbTex, v_uv[1]);
  color_1 = (color_1 + ((u_sharpenKernel[1] * tmpvar_3.xyz) * tmpvar_3.xyz));
  lowp vec4 tmpvar_4;
  tmpvar_4 = texture2D (u_rgbTex, v_uv[2]);
  color_1 = (color_1 + ((u_sharpenKernel[2] * tmpvar_4.xyz) * tmpvar_4.xyz));
  lowp vec4 tmpvar_5;
  tmpvar_5 = texture2D (u_rgbTex, v_uv[3]);
  color_1 = (color_1 + ((u_sharpenKernel[3] * tmpvar_5.xyz) * tmpvar_5.xyz));
  lowp vec4 tmpvar_6;
  tmpvar_6 = texture2D (u_rgbTex, v_uv[4]);
  color_1 = (color_1 + ((u_sharpenKernel[4] * tmpvar_6.xyz) * tmpvar_6.xyz));
  lowp vec4 tmpvar_7;
  tmpvar_7.w = 1.0;
  tmpvar_7.xyz = sqrt(max (color_1, 0.0));
  gl_FragColor = tmpvar_7;
}

