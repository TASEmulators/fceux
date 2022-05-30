uniform highp vec3 u_mins;
uniform highp vec3 u_maxs;
uniform highp float u_brightness;
uniform highp float u_contrast;
uniform highp float u_color;
uniform highp float u_gamma;
uniform sampler2D u_idxTex;
uniform sampler2D u_deempTex;
uniform sampler2D u_lookupTex;
varying highp vec2 v_uv;
varying highp vec2 v_deemp_uv;
void main ()
{
  lowp vec2 tmpvar_1;
  tmpvar_1.x = 1.0;
  tmpvar_1.y = ((0.4990215 * texture2D (u_idxTex, v_uv).x) + (0.9980431 * texture2D (u_deempTex, v_deemp_uv).x));
  lowp vec3 tmpvar_2;
  tmpvar_2 = ((texture2D (u_lookupTex, tmpvar_1).xyz * (u_maxs - u_mins)) + u_mins);
  lowp vec3 yiq_3;
  yiq_3.x = tmpvar_2.x;
  yiq_3.yz = (tmpvar_2.yz * u_color);
  lowp vec3 tmpvar_4;
  tmpvar_4 = clamp (((u_contrast * 
    pow (clamp ((mat3(1.0, 1.0, 1.0, 0.946882, -0.274788, -1.108545, 0.623557, -0.635691, 1.709007) * yiq_3), 0.0, 1.0), vec3(u_gamma))
  ) + u_brightness), 0.0, 1.0);
  mediump vec4 tmpvar_5;
  tmpvar_5.w = 1.0;
  tmpvar_5.xyz = tmpvar_4;
  gl_FragColor = tmpvar_5;
}

