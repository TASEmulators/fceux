uniform sampler2D u_idxTex;
uniform sampler2D u_deempTex;
uniform sampler2D u_lookupTex;

varying vec2 v_uv;
varying vec2 v_deemp_uv;


void main()
{
	float deemp = (255.0/511.0) * 2.0 * texture2D(u_deempTex, v_deemp_uv).r;
	float uv_y = (255.0/511.0) * texture2D(u_idxTex, v_uv).r + deemp;
	// Snatch in RGB PPU; uv.x is already calculated, so just read from lookup tex with u=1.0.
	vec3 yiq = RESCALE(texture2D(u_lookupTex, vec2(1.0, uv_y)).rgb);
	// Write linear RGB (banding is not visible for pixel graphics).
	gl_FragColor = vec4(yiq2rgb(yiq), 1.0);
}
