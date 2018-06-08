vec2 NDC(vec2 frag_coord)
{
	return (frag_coord / iResolution) * 2.0 - 1.0;
}

vec2 DefensiveAspectCoordinates(vec2 frag_coord)
{
	vec2 ndc = NDC(frag_coord);
	float vert_aspect = iResolution.x / iResolution.y;
	float inv_aspect = 1.0 / vert_aspect;
	return ndc * vec2(max(1.0, vert_aspect), max(1.0, inv_aspect));
}

vec2 VerticalAspectCoordinates(vec2 frag_coord)
{
	vec2 ndc = NDC(frag_coord);
	vec2 aspect_factor = vec2(iResolution.x / iResolution.y, 1.0);
	return ndc * aspect_factor;
}



void imageMain(inout vec4 frag_color, vec2 frag_coord)
{
	for (int i = 0; i < 4; ++i)
	{
		vec2 sample_offset = vec2(
			 0.25 * ((i < 2) ? -1.0 : 1.0),
			 0.25 * ((mod(i, 2.0) == 0.0) ? -1.0 : 1.0)
		);
		vec2 aspect_coord = VerticalAspectCoordinates(frag_coord + sample_offset);
		if (length(aspect_coord) < 1.0)
		{
			frag_color += vec4(1.0) / 4.0;
		}
		else
		{
			frag_color.xy += aspect_coord / 4.0;
		}
	}
}
