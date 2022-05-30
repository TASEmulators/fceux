attribute vec4 a_0;
attribute vec3 a_1;
attribute vec3 a_2;
attribute vec3 a_3;

uniform mat4 u_mvp;

varying vec3 v_radiance;
varying vec2 v_noiseUVs[2];
varying vec2 v_uv;
varying vec3 v_pos;
varying vec3 v_norm;
varying float v_blend;

void main()
{
	vec4 p = a_0;
// TODO: tsone: no need to normalize?
	vec3 n = normalize(a_1);
	vec3 dl = a_2;

	v_norm = a_1;
	v_pos = a_0.xyz;

	// Vertex color contains baked radiance from the TV screen.
	// Radiance from diffuse is red and specular in green component
	// (blue is unused).
	// Gamma decode and modulate by material diffuse and specular.
	v_radiance = (a_3*a_3) * vec3(0.15, 0.20, 1.0);

// TODO: tsone: precalculate the uvs and blend coeff?
	// Calculate texture coordinate for the downsampled texture sampling.
	// Fake reflection inwards to the center of the TV screen based on
	// distance to the nearest TV screen point/edge (precalculated).
	float andy = length(dl);
	float les = length(dl.xy);
	vec2 clay = (les > 0.0) ? dl.xy / les : vec2(0.0);
	vec2 nxy = normalize(n.xy);
	float mixer = max(1.0 - 30.0*les, 0.0);
	vec2 pool = normalize(mix(clay, nxy, mixer));
	vec4 tmp = u_mvp * vec4(p.xy + 5.5*vec2(1.0, 7.0/8.0)*(vec2(les) + vec2(0.0014, 0.0019))*pool, p.zw);
	v_uv = 0.5 + 0.5 * tmp.xy / tmp.w;

// TODO: tsone: duplicated (screen, tv), manual homogenization
	gl_Position = u_mvp * a_0;
	gl_Position = vec4(gl_Position.xy / gl_Position.w, 0.0, 1.0);
	vec2 vwc = gl_Position.xy * 0.5 + 0.5;
	v_noiseUVs[0] = vec2(SCREEN_W, SCREEN_H) * vwc / vec2(NOISE_W, NOISE_H);
	v_noiseUVs[1] = 0.707*v_noiseUVs[0] + 0.5 / vec2(NOISE_W, NOISE_H);

	v_blend = min(0.28*70.0 * andy, 1.0);
}
