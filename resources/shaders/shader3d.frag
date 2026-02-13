#version 330 core
out vec4 FragColor;

in vec3 normal;
in vec3 frag_pos;

uniform vec4 u_color;
uniform vec4 u_light_color;
uniform vec3 u_light_pos;
uniform vec3 u_view_pos;
uniform float u_shininess;
uniform float u_reflectivity;

void main() {
	float ambient_strength = 0.2;
	vec3 ambient = ambient_strength * u_light_color.rgb;

	vec3 norm = normalize(normal);
	vec3 light_dir = normalize(u_light_pos - frag_pos);

	float diff = max(dot(norm, light_dir), 0.0);
	vec3 diffuse = diff * u_light_color.rgb;

	vec3 view_pos = normalize(u_view_pos - frag_pos);
	vec3 refl = reflect(-light_dir, norm);
	float spec = pow(max(dot(view_pos, refl), 0.0), u_shininess);
	vec3 specular = u_reflectivity * spec * u_light_color.rgb;

	FragColor = vec4((ambient + diffuse + specular) * u_color.rgb, 1.0);
}
