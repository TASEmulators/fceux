uniform sampler2D u_tvTex;
uniform sampler2D u_downsample3Tex;
uniform sampler2D u_downsample5Tex;
uniform sampler2D u_noiseTex;
uniform vec3 u_glow;

varying vec2 v_uv;
varying vec2 v_noiseUV;

void main()
{
	// Sample screen/tv and downsampled (blurry) textures for glow.
	vec3 color = texture2D(u_tvTex, v_uv).rgb;
	vec3 ds3 = texture2D(u_downsample3Tex, v_uv).rgb;
	vec3 ds5 = texture2D(u_downsample5Tex, v_uv).rgb;
	// Linearize color values.
	color *= color;
	ds3 *= ds3;
	ds5 *= ds5;
	// Blend in glow as blurry highlight allowing slight bleeding on higher u_glow values.
	color = (color + u_glow[0]*ds3 + u_glow[1]*ds5) / (1.0 + u_glow[1]);
//	float noise = u_glow * (u_mouse.x/256.0) * (1.5/128.0) * (texture2D(u_noiseTex, v_noiseUV).r - 0.5);
//	float noise = u_glow[2] * (u_mouse.x/256.0) * (8.0/128.0) * (texture2D(u_noiseTex, v_noiseUV).r - 0.5);
	float noise = u_glow[2] * (6.0/128.0) * (texture2D(u_noiseTex, v_noiseUV).r - 0.5);
	// Gamma encode w/ sqrt() to something similar to sRGB space.
	gl_FragColor = vec4(sqrt(color) + noise, 1.0);
}
