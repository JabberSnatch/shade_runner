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

void imageMain(inout vec4 frag_color, vec2 frag_coord)
{
	vec2 local_coord = VerticalAspectCoordinates(frag_coord);
	vec3 world_position = vec3(local_coord * 10.0 + vec2(0.5), 0.0);
	float distance = 1.0;
	distance = object(fract(-(world_position).xyz), 0.09);
	float intensity = sin(iTime - length(floor(world_position)));
	frag_color = vec4(max(0.0, -distance * 20.0) * intensity);
}