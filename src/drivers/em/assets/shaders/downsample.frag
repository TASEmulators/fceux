uniform sampler2D u_downsampleTex;
uniform float u_weights[8];

varying vec2 v_uv[8];

void main()
{
	vec3 result = vec3(0.0);
	for (int i = 0; i < 8; i++) {
		vec3 color = texture2D(u_downsampleTex, v_uv[i]).rgb;
		result += u_weights[i] * color*color;
	}
	gl_FragColor = vec4(sqrt(result), 1.0);
}
