float rand(float n){return fract(sin(n) * 43758.5453123);}

float rand(vec2 n)
{
	return fract(sin(dot(n, vec2(12.9898, 4.1414))) * 43758.5453);
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


float udRoundBox(vec3 p, vec3 b, float r)
{
	return length(max(abs(p)-b, 0.0))-r;
}

float sdSphere(vec3 p, float r)
{
	return length(p) - r;
}

float udRoundSegment(vec3 p, vec3 start, vec3 end, float r)
{
	vec3 extent = (end - start) * 0.5;
	vec3 center = (end + start) * 0.5;
	return udRoundBox(p - center, extent, r);
}


float object(vec3 p, float r)
{
	float distance = udRoundSegment(p, vec3(0.25, 0.25, 0.0), vec3(0.25, 0.75, 0.0), r);
	distance = min(distance, udRoundSegment(p, vec3(0.75, 0.25, 0.0), vec3(0.75, 0.75, 0.0), r));
	distance = min(distance, udRoundSegment(p, vec3(0.5, 0.25, 0.0), vec3(0.5, 0.5, 0.0), r));
	distance = min(distance, udRoundSegment(p, vec3(0.5, 0.5, 0.0), vec3(0.75, 0.5, 0.0), r));
	distance = min(distance, udRoundSegment(p, vec3(0.5, 0.75, 0.0), vec3(0.5, 0.75, 0.0), r));
	return distance;
}

float rand_object(vec3 p, float rand_seed)
{
	float points[3] = float[3](0.18, 0.5, 0.82);
	float radius = 0.11;
	float segment_p = 0.5;

	float rand_state = rand_seed;
	float distance = 1. / 0.;
	for (int i = 0; i < 3; ++i)
	{
		for (int j = 0; j < 3; ++j)
		{
			distance = min(distance, sdSphere(p - vec3(points[i], points[j], 0.0), radius));
		}
	}
	for (int i = 0; i < 6; ++i)
	{
		float constant_point = points[i/2];
		int start_index = int(mod(float(i), 2.0));
		vec2 start = vec2(points[start_index], constant_point);
		vec2 end = vec2(points[start_index + 1], constant_point);
		if (rand_state < segment_p)
		{
			distance = min(distance, udRoundSegment(p, vec3(start, 0.0), vec3(end, 0.0), radius));
		}
		else if (rand_state > segment_p)
		{
			distance = min(distance, udRoundSegment(p, vec3(start.yx, 0.0), vec3(end.yx, 0.0), radius));
		}
		rand_state = rand(rand_state);
	}
	return distance;
}

void imageMain(inout vec4 frag_color, vec2 frag_coord)
{
	float scale = 15.0;
	vec3 glyph_color = vec3(0.67, 0.65, 0.42);
	vec3 background_color = vec3(0.05, 0.04, 0.03);

	vec2 local_coord = VerticalAspectCoordinates(frag_coord);
	vec3 world_position = vec3(local_coord * scale + vec2(0.5), 0.0) + vec3(vec2(iTime), 0.0);
	vec3 floor_wp = floor(world_position);

	float intensity = sin((iTime + rand(floor_wp.xy) * 4.0) * (rand(floor_wp.yx) + 0.5));
	float distance = 1.0;
	distance = rand_object(fract(world_position.xyz), rand(floor_wp.xy + floor(iTime * 0.2 + rand(floor_wp.yx))));

#if 0
#if 1
	intensity = max(0.0, -distance * 15.0) * intensity;
#else
	if (distance > 0.0)
	{
		intensity = 0.0;
	}
#endif
	intensity = clamp(intensity, 0.0, 1.0);
	frag_color = vec4(mix(abs(distance) * background_color * 20.0, glyph_color, intensity), 1.0);
#else
	intensity = clamp(intensity, 0.0, 1.0);

	float pixel_size = scale * (2.0 / iResolution.y);
	float blur_radius = pixel_size;
	float shape_alpha = clamp(-distance / blur_radius, 0.0, 1.0);

	frag_color = vec4(mix(abs(distance) * background_color * 20.0, glyph_color, shape_alpha * intensity), 1.0);
#endif
}