uniform sampler2D u_tvTex;
uniform sampler2D u_downsample3Tex;
uniform sampler2D u_downsample5Tex;
uniform sampler2D u_noiseTex;
uniform highp vec3 u_glow;
varying highp vec2 v_uv;
varying highp vec2 v_noiseUV;
void main ()
{
  lowp vec3 color_1;
  lowp vec4 tmpvar_2;
  tmpvar_2 = texture2D (u_tvTex, v_uv);
  lowp vec4 tmpvar_3;
  tmpvar_3 = texture2D (u_downsample3Tex, v_uv);
  lowp vec4 tmpvar_4;
  tmpvar_4 = texture2D (u_downsample5Tex, v_uv);
  color_1 = (tmpvar_2.xyz * tmpvar_2.xyz);
  color_1 = (((color_1 + 
    (u_glow.x * (tmpvar_3.xyz * tmpvar_3.xyz))
  ) + (u_glow.y * 
    (tmpvar_4.xyz * tmpvar_4.xyz)
  )) / (1.0 + u_glow.y));
  lowp vec4 tmpvar_5;
  tmpvar_5.w = 1.0;
  tmpvar_5.xyz = (sqrt(color_1) + ((u_glow.z * 0.046875) * (texture2D (u_noiseTex, v_noiseUV).x - 0.5)));
  gl_FragColor = tmpvar_5;
}

