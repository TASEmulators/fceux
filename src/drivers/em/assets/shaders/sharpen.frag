uniform sampler2D u_rgbTex;
uniform float u_convergence;
uniform vec3 u_sharpenKernel[5];

varying vec2 v_uv[5];

void main()
{
	vec3 color = vec3(0.0);
	// Sharpening in linear color space.
	for (int i = 0; i < 5; ++i) {
		vec3 tmp = texture2D(u_rgbTex, v_uv[i]).rgb;
		color += u_sharpenKernel[i] * tmp*tmp;
	}
	gl_FragColor = vec4(sqrt(max(color, 0.0)), 1.0);
}
