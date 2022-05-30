attribute vec4 a_0;
attribute vec3 a_1;
attribute vec3 a_2;
attribute vec3 a_3;
uniform mat4 u_mvp;
varying vec3 v_radiance;
varying highp vec2 v_noiseUVs[2];
varying vec2 v_uv;
varying vec3 v_pos;
varying vec3 v_norm;
varying float v_blend;
void main ()
{
  vec3 tmpvar_1;
  tmpvar_1 = normalize(a_1);
  v_norm = a_1;
  v_pos = a_0.xyz;
  v_radiance = ((a_3 * a_3) * vec3(0.15, 0.2, 1.0));
  float tmpvar_2;
  tmpvar_2 = sqrt(dot (a_2, a_2));
  float tmpvar_3;
  tmpvar_3 = sqrt(dot (a_2.xy, a_2.xy));
  vec2 tmpvar_4;
  if ((tmpvar_3 > 0.0)) {
    tmpvar_4 = (a_2.xy / tmpvar_3);
  } else {
    tmpvar_4 = vec2(0.0, 0.0);
  };
  vec4 tmpvar_5;
  tmpvar_5.xy = (a_0.xy + ((vec2(5.5, 4.8125) * 
    (vec2(tmpvar_3) + vec2(0.0014, 0.0019))
  ) * normalize(
    mix (tmpvar_4, normalize(tmpvar_1.xy), max ((1.0 - (30.0 * tmpvar_3)), 0.0))
  )));
  tmpvar_5.zw = a_0.zw;
  vec4 tmpvar_6;
  tmpvar_6 = (u_mvp * tmpvar_5);
  v_uv = (0.5 + ((0.5 * tmpvar_6.xy) / tmpvar_6.w));
  gl_Position = (u_mvp * a_0);
  highp vec4 tmpvar_7;
  tmpvar_7.zw = vec2(0.0, 1.0);
  tmpvar_7.xy = (gl_Position.xy / gl_Position.w);
  gl_Position = tmpvar_7;
  v_noiseUVs[0] = ((vec2(1024.0, 960.0) * (
    (tmpvar_7.xy * 0.5)
   + 0.5)) / vec2(256.0, 256.0));
  v_noiseUVs[1] = ((0.707 * v_noiseUVs[0]) + vec2(0.001953125, 0.001953125));
  v_blend = min ((19.6 * tmpvar_2), 1.0);
}

