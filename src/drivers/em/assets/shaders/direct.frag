uniform sampler2D u_tex;

varying vec2 v_uv;

void main()
{
	gl_FragColor = texture2D(u_tex, v_uv);
}
