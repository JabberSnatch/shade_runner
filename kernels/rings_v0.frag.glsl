
const float kPi = 3.1415926535;

float rand(vec2 n)
{
	return fract(sin(dot(n, vec2(12.9898, 4.1414))) * 43758.5453);
}

vec3 HSLtoRGB(vec3 hsl)
{
	vec3 hue = clamp(vec3(
		abs(hsl.x * 6.0 - 3.0) - 1.0,
		2.0 - abs(hsl.x * 6.0 - 2.0),
		2.0 - abs(hsl.x * 6.0 - 4.0)), 0.0, 1.0);
	float C = (1.0 - abs(2.0 * hsl.z - 1.0)) * hsl.y;
	return (hue - vec3(0.5)) * C + hsl.zzz;
}

vec2 NDC(vec2 frag_coord)
{
	return (frag_coord / iResolution) * 2.0 - 1.0;
}

vec2 VerticalAspectCoordinates(vec2 frag_coord)
{
	vec2 ndc = NDC(frag_coord);
	vec2 aspect_factor = vec2(iResolution.x / iResolution.y, 1.0);
	return ndc * aspect_factor;
}


void imageMain(inout vec4 frag_color, vec2 frag_coord)
{
	vec2 p = VerticalAspectCoordinates(frag_coord);

	float time_sine = sin(iTime) * 0.5 + 0.5;

	float scale = 4.0;
	float cutoff_size = 0.9 / scale;
	vec2 center = p / scale;

	vec2 local_p = p - center;
	float raw_length = length(local_p);
	float distance = (raw_length - iTime * 0.5) * scale * 3.0;
	float eye_radius = 0.1 / scale;
	float eye_region = 1.0 - step(eye_radius, raw_length);

	float radius = floor(distance);
	float rotation_speed_bound = 1.0;
	float segment_count = 6.0;
	float angle = (atan(local_p.x, local_p.y) / kPi) * 0.5 + 0.5;
	angle *= segment_count;
	angle += iTime * mix(-rotation_speed_bound, rotation_speed_bound, rand(vec2(radius, 0.2)));

	float r = step(0.2, fract(distance));
	float threshold = 1.0 - (time_sine * 0.35 + 0.15);
	float theta = max(
		  step(threshold, fract(angle)),
		  step(threshold, 1.0 - fract(angle))
	);

	float ring_index = floor(distance);
	float hue = fract(ring_index * 31.0 / 13.0);
	vec3 ring_color = HSLtoRGB(vec3(hue, 1.0, 0.5));
	ring_color *= r * theta * fract(distance) * (1.0 - eye_region);
	vec3 eye_color = vec3(1.0) * mix(0.0, 1.0, raw_length / eye_radius) * eye_region;

	frag_color = vec4(
			   ring_color + eye_color,
			   1.0
	);
	frag_color *= 1.0 - max(step(cutoff_size, abs(local_p.x)), step(cutoff_size, abs(local_p.y)));
}