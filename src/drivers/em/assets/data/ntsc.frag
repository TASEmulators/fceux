uniform highp vec3 u_mins;
uniform highp vec3 u_maxs;
uniform highp float u_brightness;
uniform highp float u_contrast;
uniform highp float u_color;
uniform highp float u_gamma;
uniform highp float u_noiseAmp;
uniform sampler2D u_idxTex;
uniform sampler2D u_deempTex;
uniform sampler2D u_lookupTex;
uniform sampler2D u_noiseTex;
varying vec2 v_uv[5];
varying highp vec2 v_deemp_uv;
varying highp vec2 v_noiseUV;
void main ()
{
  lowp vec3 yiq_1;
  lowp float tmpvar_2;
  tmpvar_2 = (0.9980431 * texture2D (u_deempTex, v_deemp_uv).x);
  highp float tmpvar_3;
  tmpvar_3 = (float(mod (floor((1120.0 * v_uv[2].x)), 4.0)));
  highp vec2 tmpvar_4;
  tmpvar_4 = floor((vec2(280.0, 240.0) * v_uv[0]));
  lowp vec2 tmpvar_5;
  tmpvar_5.x = (((20.0 * 
    (float(mod ((tmpvar_4.x - tmpvar_4.y), 3.0)))
  ) + (tmpvar_3 * 5.0)) / 63.0);
  tmpvar_5.y = ((0.4990215 * texture2D (u_idxTex, v_uv[0]).x) + tmpvar_2);
  highp vec3 tmpvar_6;
  tmpvar_6 = (u_maxs - u_mins);
  yiq_1 = ((texture2D (u_lookupTex, tmpvar_5).xyz * tmpvar_6) + u_mins);
  highp vec2 tmpvar_7;
  tmpvar_7 = floor((vec2(280.0, 240.0) * v_uv[1]));
  lowp vec2 tmpvar_8;
  tmpvar_8.x = (((
    (20.0 * (float(mod ((tmpvar_7.x - tmpvar_7.y), 3.0))))
   + 
    (tmpvar_3 * 5.0)
  ) + 1.0) / 63.0);
  tmpvar_8.y = ((0.4990215 * texture2D (u_idxTex, v_uv[1]).x) + tmpvar_2);
  yiq_1 = (yiq_1 + ((texture2D (u_lookupTex, tmpvar_8).xyz * tmpvar_6) + u_mins));
  highp vec2 tmpvar_9;
  tmpvar_9 = floor((vec2(280.0, 240.0) * v_uv[2]));
  lowp vec2 tmpvar_10;
  tmpvar_10.x = (((
    (20.0 * (float(mod ((tmpvar_9.x - tmpvar_9.y), 3.0))))
   + 
    (tmpvar_3 * 5.0)
  ) + 2.0) / 63.0);
  tmpvar_10.y = ((0.4990215 * texture2D (u_idxTex, v_uv[2]).x) + tmpvar_2);
  yiq_1 = (yiq_1 + ((texture2D (u_lookupTex, tmpvar_10).xyz * tmpvar_6) + u_mins));
  highp vec2 tmpvar_11;
  tmpvar_11 = floor((vec2(280.0, 240.0) * v_uv[3]));
  lowp vec2 tmpvar_12;
  tmpvar_12.x = (((
    (20.0 * (float(mod ((tmpvar_11.x - tmpvar_11.y), 3.0))))
   + 
    (tmpvar_3 * 5.0)
  ) + 3.0) / 63.0);
  tmpvar_12.y = ((0.4990215 * texture2D (u_idxTex, v_uv[3]).x) + tmpvar_2);
  yiq_1 = (yiq_1 + ((texture2D (u_lookupTex, tmpvar_12).xyz * tmpvar_6) + u_mins));
  highp vec2 tmpvar_13;
  tmpvar_13 = floor((vec2(280.0, 240.0) * v_uv[4]));
  lowp vec2 tmpvar_14;
  tmpvar_14.x = (((
    (20.0 * (float(mod ((tmpvar_13.x - tmpvar_13.y), 3.0))))
   + 
    (tmpvar_3 * 5.0)
  ) + 4.0) / 63.0);
  tmpvar_14.y = ((0.4990215 * texture2D (u_idxTex, v_uv[4]).x) + tmpvar_2);
  yiq_1 = (yiq_1 + ((texture2D (u_lookupTex, tmpvar_14).xyz * tmpvar_6) + u_mins));
  yiq_1 = (yiq_1 * vec3(0.6666667, 0.4, 0.4));
  yiq_1.x = (yiq_1.x + (u_noiseAmp * (texture2D (u_noiseTex, v_noiseUV).x - 0.5)));
  lowp vec3 yiq_15;
  yiq_15.x = yiq_1.x;
  yiq_15.yz = (yiq_1.yz * u_color);
  lowp vec3 tmpvar_16;
  tmpvar_16 = clamp (((u_contrast * 
    pow (clamp ((mat3(1.0, 1.0, 1.0, 0.946882, -0.274788, -1.108545, 0.623557, -0.635691, 1.709007) * yiq_15), 0.0, 1.0), vec3(u_gamma))
  ) + u_brightness), 0.0, 1.0);
  mediump vec4 tmpvar_17;
  tmpvar_17.w = 1.0;
  tmpvar_17.xyz = tmpvar_16;
  gl_FragColor = tmpvar_17;
}

