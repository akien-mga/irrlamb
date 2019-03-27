varying vec3 normal;
varying vec3 vertex;
uniform sampler2D texture;
uniform int light_count;

void main(void) {

	// Calculate light color
	vec4 light_color = vec4(0, 0, 0, 0);
	for(int i = 0; i < light_count; i++) {

		// Get distance from light
		float distance = length(gl_LightSource[i].position.xyz - vertex);

		// Attenuate
		float attenuation = 1.0 / (gl_LightSource[i].constantAttenuation + distance * gl_LightSource[i].linearAttenuation + distance * distance * gl_LightSource[i].quadraticAttenuation);
		attenuation = clamp(attenuation, 0.0, 1.0);

		// Get light vector
		vec3 light_vector = normalize(gl_LightSource[i].position.xyz - vertex);

		// Diffuse
		vec4 diffuse = gl_LightSource[i].diffuse * max(dot(normal, light_vector), 0.0);
		light_color += attenuation * diffuse;
	}
	light_color.w = 0.0;

	// Ambient
	vec4 ambient = vec4(gl_LightModel.ambient.x, gl_LightModel.ambient.y, gl_LightModel.ambient.z, 1);

	// Texture color
	vec4 texture_color = texture2D(texture, vec2(gl_TexCoord[0]));

	// Final color
	vec4 frag_color = texture_color * (ambient + light_color);
	//vec4 frag_color = vec4(0, 0, 0, 1);

	// Fog
	if(gl_Fog.density > 0.0) {
		const float LOG2 = 1.442695;
		float frag_z = gl_FragCoord.z / gl_FragCoord.w;
		float fog_factor = clamp(exp2(-gl_Fog.density * gl_Fog.density * frag_z * frag_z * LOG2), 0.0, 1.0);

		gl_FragColor = mix(gl_Fog.color, frag_color, fog_factor);
	}
	else {
		gl_FragColor = frag_color;
	}
}
