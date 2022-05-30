uniform sampler2D u_downsampleTex;
uniform float u_weights[8];
varying vec2 v_uv[8];
void main ()
{
  lowp vec3 result_1;
  lowp vec4 tmpvar_2;
  tmpvar_2 = texture2D (u_downsampleTex, v_uv[0]);
  result_1 = ((u_weights[0] * tmpvar_2.xyz) * tmpvar_2.xyz);
  lowp vec4 tmpvar_3;
  tmpvar_3 = texture2D (u_downsampleTex, v_uv[1]);
  result_1 = (result_1 + ((u_weights[1] * tmpvar_3.xyz) * tmpvar_3.xyz));
  lowp vec4 tmpvar_4;
  tmpvar_4 = texture2D (u_downsampleTex, v_uv[2]);
  result_1 = (result_1 + ((u_weights[2] * tmpvar_4.xyz) * tmpvar_4.xyz));
  lowp vec4 tmpvar_5;
  tmpvar_5 = texture2D (u_downsampleTex, v_uv[3]);
  result_1 = (result_1 + ((u_weights[3] * tmpvar_5.xyz) * tmpvar_5.xyz));
  lowp vec4 tmpvar_6;
  tmpvar_6 = texture2D (u_downsampleTex, v_uv[4]);
  result_1 = (result_1 + ((u_weights[4] * tmpvar_6.xyz) * tmpvar_6.xyz));
  lowp vec4 tmpvar_7;
  tmpvar_7 = texture2D (u_downsampleTex, v_uv[5]);
  result_1 = (result_1 + ((u_weights[5] * tmpvar_7.xyz) * tmpvar_7.xyz));
  lowp vec4 tmpvar_8;
  tmpvar_8 = texture2D (u_downsampleTex, v_uv[6]);
  result_1 = (result_1 + ((u_weights[6] * tmpvar_8.xyz) * tmpvar_8.xyz));
  lowp vec4 tmpvar_9;
  tmpvar_9 = texture2D (u_downsampleTex, v_uv[7]);
  result_1 = (result_1 + ((u_weights[7] * tmpvar_9.xyz) * tmpvar_9.xyz));
  lowp vec4 tmpvar_10;
  tmpvar_10.w = 1.0;
  tmpvar_10.xyz = sqrt(result_1);
  gl_FragColor = tmpvar_10;
}

