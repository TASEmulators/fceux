uniform vec3 u_lightDir;
uniform vec3 u_viewPos;
uniform vec4 u_material;
uniform vec3 u_fresnel;
uniform vec4 u_shadowPlane;
uniform vec2 u_borderUVOffs;

uniform sampler2D u_stretchTex;
uniform sampler2D u_noiseTex;

varying vec2 v_uv;
varying vec3 v_norm;
varying vec3 v_pos;
varying vec2 v_noiseUV;

void main()
{
	// Base radiance from phosphors.
	vec3 color = texture2D(u_stretchTex, v_uv).rgb;
// TODO: test encode gamma
	color *= color;
	// Radiance from dust scattering.
	vec3 N = normalize(v_norm);
// TODO: test encode gamma
	vec3 tmp = texture2D(u_stretchTex, v_uv - 0.021*N.xy).rgb;
	color += 0.018 * tmp*tmp;
//	color += 0.018 * texture2D(u_stretchTex, v_uv - 0.021*N.xy).rgb;

	// Set black if outside the border
	vec2 uvd = max(abs(v_uv - 0.5) - u_borderUVOffs, 0.0);
	float border = clamp(3.0 - 3.0*3000.0 * dot(uvd, uvd), 0.0, 1.0);
	color *= border;

	vec3 V = normalize(u_viewPos - v_pos);
	float shade = shadeBlinn(V, N, u_lightDir, u_fresnel, u_material);
	shade *= fakeShadow(v_pos, u_shadowPlane);
	// Shading affected by black screen border material.
	shade *= 0.638*border + 0.362;

	// CRT emitter radiance attenuation from the curvature
//	color *= pow(dot(N, V), 16.0*(u_mouse.x/256.0));

	float noise = (1.5/128.0) * (texture2D(u_noiseTex, v_noiseUV).r - 0.5);

	// Use gamma encoding as we may output smooth color changes here.
	gl_FragColor = vec4(sqrt(color + shade) + noise, 1.0);
//	gl_FragColor = vec4(vec3(2.0*abs((128.0/1.5) * noise)), 1.0);
//	gl_FragColor = vec4(vec3(texture2D(u_noiseTex, v_noiseUV).r), 1.0);
//	gl_FragColor = vec4(vec3(texture2D(u_noiseTex, gl_FragCoord.xy/256.0 - 2.0).r), 1.0);
}
