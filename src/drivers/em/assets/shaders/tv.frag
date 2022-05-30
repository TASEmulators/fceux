uniform vec3 u_lightDir;
uniform vec3 u_viewPos;
uniform vec4 u_material;
uniform vec3 u_fresnel;
uniform vec4 u_shadowPlane;

uniform sampler2D u_downsample1Tex;
uniform sampler2D u_downsample3Tex;
uniform sampler2D u_downsample5Tex;
uniform sampler2D u_noiseTex;

varying vec3 v_radiance;
varying vec2 v_noiseUVs[2];
varying vec3 v_pos;
varying vec3 v_norm;
varying vec2 v_uv;
varying float v_blend;

void main()
{
	// Sample noise at low and high frequencies.
	float noiseLF = texture2D(u_noiseTex, v_noiseUVs[1]).r;
	float noiseHF = texture2D(u_noiseTex, v_noiseUVs[0]).r;

	// Sample downsampled (blurry) textures and gamma decode.
	vec3 ds0 = texture2D(u_downsample1Tex, v_uv).rgb;
	vec3 ds1 = texture2D(u_downsample3Tex, v_uv).rgb;
	vec3 ds2 = texture2D(u_downsample5Tex, v_uv).rgb;
	ds0 *= ds0;
	ds1 *= ds1;
	ds2 *= ds2;

	vec3 color;

	// Approximate diffuse and specular by sampling downsampled texture and
	// blending according to the fragment distance from the TV screen (emitter).
	color = v_radiance.r * mix(ds1, ds2, v_blend);
	color += v_radiance.g * mix(ds0, ds1, v_blend);

	// Add slight graininess for rough plastic (also to hide interpolation artifacts).
	float graininess = 0.15625 * v_blend + 0.14578;
	color *= 1.0 - graininess * sqrt(abs(2.0*noiseLF - 1.0));

	vec3 N = normalize(v_norm);
	vec3 V = normalize(u_viewPos - v_pos);
	float shade = shadeBlinn(V, N, u_lightDir, u_fresnel, u_material);
	shade *= fakeShadow(v_pos, u_shadowPlane);

// TODO: tsone: tone-mapping?

// TODO: tsone: vignette?
//	color = vec3(0.0);
//	vec2 nuv = 2.0*v_uv - 1.0;
//	float vignette = max(1.0 - length(nuv), 0.0);
//	vec3 lightColor = vec3(v_shade);

	// Gamma encode color w/ sqrt().
	gl_FragColor = vec4(sqrt(color + shade) + (1.5/128.0) * (noiseHF-0.5), 1.0);
}
