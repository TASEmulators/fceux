uniform sampler2D u_tex;
varying highp vec2 v_uv;
void main ()
{
  lowp vec4 tmpvar_1;
  tmpvar_1 = texture2D (u_tex, v_uv);
  gl_FragColor = tmpvar_1;
}

