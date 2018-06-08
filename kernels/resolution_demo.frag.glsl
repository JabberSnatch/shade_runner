
void imageMain(inout vec4 frag_color, vec2 frag_coord)
{
	vec2 uv = frag_coord / iResolution;
	float aspect_ratio = iResolution.x / iResolution.y;
	uv.x = uv.x * aspect_ratio;
	uv = frag_coord / iResolution.y;
	vec2 position = uv * 2.0 - 1.0;
	if (length(position) < 1.0)
	   frag_color = vec4(1.0);
}