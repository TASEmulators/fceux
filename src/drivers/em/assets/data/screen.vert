attribute vec4 a_0;
attribute vec3 a_1;
attribute vec2 a_2;
uniform mat4 u_mvp;
uniform vec2 u_uvScale;
varying vec2 v_uv;
varying vec3 v_norm;
varying vec3 v_pos;
varying highp vec2 v_noiseUV;
void main ()
{
  v_uv = (0.5 + (u_uvScale * (a_2 - 0.5)));
  v_norm = a_1;
  v_pos = a_0.xyz;
  gl_Position = (u_mvp * a_0);
  highp vec4 tmpvar_1;
  tmpvar_1.zw = vec2(0.0, 1.0);
  tmpvar_1.xy = (gl_Position.xy / gl_Position.w);
  gl_Position = tmpvar_1;
  v_noiseUV = ((vec2(1024.0, 960.0) * (
    (tmpvar_1.xy * 0.5)
   + 0.5)) / vec2(256.0, 256.0));
}

