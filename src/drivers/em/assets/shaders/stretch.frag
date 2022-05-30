uniform float u_scanlines;
uniform sampler2D u_sharpenTex;

varying vec2 v_uv[2];

void main()
{
	// Sample adjacent scanlines and average to smoothen slightly vertically.
	vec3 c0 = texture2D(u_sharpenTex, v_uv[0]).rgb;
	vec3 c1 = texture2D(u_sharpenTex, v_uv[1]).rgb;
	vec3 color = c0*c0 + c1*c1; // Average linear-space color values.
//	vec3 color = c0 + c1;
	// Use oscillator as scanline modulator.
	float scanlines = u_scanlines * (1.0 - abs(sin(M_PI*IDX_H * v_uv[0].y - M_PI*0.125)));
	// This formula dims dark colors, but keeps brights. Output is linear.
	gl_FragColor = vec4(sqrt(mix(0.5 * color, max(color - 1.0, 0.0), scanlines)), 1.0);
//	gl_FragColor = vec4(mix(0.5 * color, max(color - 1.0, 0.0), scanlines), 1.0);
}
