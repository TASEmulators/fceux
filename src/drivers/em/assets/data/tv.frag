uniform highp vec3 u_lightDir;
uniform highp vec3 u_viewPos;
uniform highp vec4 u_material;
uniform highp vec3 u_fresnel;
uniform highp vec4 u_shadowPlane;
uniform sampler2D u_downsample1Tex;
uniform sampler2D u_downsample3Tex;
uniform sampler2D u_downsample5Tex;
uniform sampler2D u_noiseTex;
varying highp vec3 v_radiance;
varying vec2 v_noiseUVs[2];
varying highp vec3 v_pos;
varying highp vec3 v_norm;
varying highp vec2 v_uv;
varying highp float v_blend;
void main ()
{
  lowp vec3 color_1;
  lowp vec3 ds1_2;
  lowp float noiseHF_3;
  noiseHF_3 = texture2D (u_noiseTex, v_noiseUVs[0]).x;
  lowp vec4 tmpvar_4;
  tmpvar_4 = texture2D (u_downsample1Tex, v_uv);
  lowp vec4 tmpvar_5;
  tmpvar_5 = texture2D (u_downsample3Tex, v_uv);
  lowp vec4 tmpvar_6;
  tmpvar_6 = texture2D (u_downsample5Tex, v_uv);
  ds1_2 = (tmpvar_5.xyz * tmpvar_5.xyz);
  color_1 = (v_radiance.x * mix (ds1_2, (tmpvar_6.xyz * tmpvar_6.xyz), v_blend));
  color_1 = (color_1 + (v_radiance.y * mix (
    (tmpvar_4.xyz * tmpvar_4.xyz)
  , ds1_2, v_blend)));
  color_1 = (color_1 * (1.0 - (
    ((0.15625 * v_blend) + 0.14578)
   * 
    sqrt(abs(((2.0 * texture2D (u_noiseTex, v_noiseUVs[1]).x) - 1.0)))
  )));
  highp vec3 N_7;
  N_7 = normalize(v_norm);
  highp float tmpvar_8;
  highp vec3 tmpvar_9;
  tmpvar_9 = normalize((u_lightDir + normalize(
    (u_viewPos - v_pos)
  )));
  highp float tmpvar_10;
  tmpvar_10 = dot (N_7, u_lightDir);
  if ((tmpvar_10 > 0.0)) {
    tmpvar_8 = (mix (u_material.x, (u_material.y * 
      pow (max (dot (N_7, tmpvar_9), 0.0), u_material.z)
    ), (u_fresnel.x + 
      (u_fresnel.y * pow ((1.0 - tmpvar_10), u_fresnel.z))
    )) * tmpvar_10);
  } else {
    tmpvar_8 = (-(tmpvar_10) * u_material.w);
  };
  lowp vec4 tmpvar_11;
  tmpvar_11.w = 1.0;
  tmpvar_11.xyz = (sqrt((color_1 + 
    (tmpvar_8 * (0.21 + (0.79 * clamp (
      (38.0 * max ((dot (u_shadowPlane.xyz, v_pos) - u_shadowPlane.w), (v_pos.z - 0.023)))
    , 0.0, 1.0))))
  )) + (0.01171875 * (noiseHF_3 - 0.5)));
  gl_FragColor = tmpvar_11;
}

