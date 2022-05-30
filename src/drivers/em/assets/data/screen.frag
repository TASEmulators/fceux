uniform highp vec3 u_lightDir;
uniform highp vec3 u_viewPos;
uniform highp vec4 u_material;
uniform highp vec3 u_fresnel;
uniform highp vec4 u_shadowPlane;
uniform highp vec2 u_borderUVOffs;
uniform sampler2D u_stretchTex;
uniform sampler2D u_noiseTex;
varying highp vec2 v_uv;
varying highp vec3 v_norm;
varying highp vec3 v_pos;
varying highp vec2 v_noiseUV;
void main ()
{
  highp float shade_1;
  lowp vec3 color_2;
  lowp vec4 tmpvar_3;
  tmpvar_3 = texture2D (u_stretchTex, v_uv);
  color_2 = (tmpvar_3.xyz * tmpvar_3.xyz);
  highp vec3 tmpvar_4;
  tmpvar_4 = normalize(v_norm);
  lowp vec4 tmpvar_5;
  tmpvar_5 = texture2D (u_stretchTex, (v_uv - (0.021 * tmpvar_4.xy)));
  color_2 = (color_2 + ((0.018 * tmpvar_5.xyz) * tmpvar_5.xyz));
  highp vec2 tmpvar_6;
  tmpvar_6 = max ((abs(
    (v_uv - 0.5)
  ) - u_borderUVOffs), 0.0);
  highp float tmpvar_7;
  tmpvar_7 = clamp ((3.0 - (9000.0 * 
    dot (tmpvar_6, tmpvar_6)
  )), 0.0, 1.0);
  color_2 = (color_2 * tmpvar_7);
  highp float tmpvar_8;
  highp vec3 tmpvar_9;
  tmpvar_9 = normalize((u_lightDir + normalize(
    (u_viewPos - v_pos)
  )));
  highp float tmpvar_10;
  tmpvar_10 = dot (tmpvar_4, u_lightDir);
  if ((tmpvar_10 > 0.0)) {
    tmpvar_8 = (mix (u_material.x, (u_material.y * 
      pow (max (dot (tmpvar_4, tmpvar_9), 0.0), u_material.z)
    ), (u_fresnel.x + 
      (u_fresnel.y * pow ((1.0 - tmpvar_10), u_fresnel.z))
    )) * tmpvar_10);
  } else {
    tmpvar_8 = (-(tmpvar_10) * u_material.w);
  };
  shade_1 = (tmpvar_8 * (0.21 + (0.79 * 
    clamp ((38.0 * max ((
      dot (u_shadowPlane.xyz, v_pos)
     - u_shadowPlane.w), (v_pos.z - 0.023))), 0.0, 1.0)
  )));
  shade_1 = (shade_1 * ((0.638 * tmpvar_7) + 0.362));
  lowp vec4 tmpvar_11;
  tmpvar_11.w = 1.0;
  tmpvar_11.xyz = (sqrt((color_2 + shade_1)) + (0.01171875 * (texture2D (u_noiseTex, v_noiseUV).x - 0.5)));
  gl_FragColor = tmpvar_11;
}

