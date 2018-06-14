
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

vec2 PixelAspectCoordinates(vec2 frag_coord, float pixel_count)
{
	vec2 ndc = NDC(frag_coord);
	float pixel_factor = iResolution.y * 0.5 / pixel_count;
	vec2 aspect_factor = vec2(iResolution.x / iResolution.y, 1.0) * pixel_factor;
	return ndc * aspect_factor;
}

void imageMain(inout vec4 frag_color, vec2 frag_coord)
{
#if 1
	vec2 p = VerticalAspectCoordinates(frag_coord);
	float scale = 4.0;
	p *= scale;
#else
	vec2 p = PixelAspectCoordinates(frag_coord, 75.0);
#endif

	p += vec2(0.5);

	float local_rand = rand(floor(p));
	vec2 local_center = (floor(p) + vec2(0.5));
	vec2 local_p = p - local_center;
	vec2 ring_center = local_center + vec2(sin(iTime) * 0.1, cos(iTime*2.0) * 0.3);
	vec2 ring_p = p - ring_center;
	float raw_length = length(ring_p);
	float distance = (raw_length - iTime * 0.8 - length(floor(p)) * 0.2);
	float eye_radius = 0.07;
	float eye_region = 1.0 - step(eye_radius, raw_length);

	distance *= 6.0;

	float ring_index = floor(distance);
	float rotation_speed_bound = 1.0;
	float segment_count = 3.0;
	float angle = (atan(ring_p.x, ring_p.y) / kPi) * 0.5 + 0.5;
	angle *= segment_count;
	angle += iTime * mix(-rotation_speed_bound, rotation_speed_bound, rand(vec2(ring_index, 0.2)));

	float r = step(0.2, fract(distance));
	float time_sine = sin(iTime * 3.0) * 0.5 + 0.5;
	float threshold = 1.0 - (time_sine * 0.2 + 0.2);
	float theta = max(
		  step(threshold, fract(angle)),
		  step(threshold, 1.0 - fract(angle))
	);

	float hue = fract((ring_index * 2.0 / 7.0));
	vec3 ring_color = HSLtoRGB(vec3(hue, 0.8, 0.5));
	ring_color *= r * theta * clamp(log(raw_length * 10.0), 0.0, 1.0);
	vec3 eye_color = vec3(1.0) * mix(1.0, 0.2, raw_length / eye_radius);

	frag_color = vec4(
			   mix(ring_color, eye_color, eye_region),
			   1.0
	);

#if 1
	float cutoff_size = 0.47;
	float cutoff_radius = 0.15;
	float cutoff_factor = 1.0 - max(max(
		  step(cutoff_size, abs(local_p.x)),
		  step(cutoff_size, abs(local_p.y))),
		  step(cutoff_radius, length(abs(local_p)) - cutoff_size)
	);
	frag_color = mix(vec4(0.12, 0.1, 0.04, 1.0) * 8.0, frag_color, cutoff_factor);
#endif
}