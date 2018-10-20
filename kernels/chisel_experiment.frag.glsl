
float sdPlane(vec3 p, vec3 n, vec3 o)
{
	vec3 local_p = o - p;
	return dot(local_p, n);
}

void imageMain(inout vec4 frag_color, vec2 frag_coord)
{
	vec3 planes_o = vec3(0.0, 0.0, 0.0);
	vec3 top_n = normalize(vec3(0.0, -1.0, -1.0));
	vec3 bottom_n = normalize(vec3(0.0, 1.0, -1.0));
	vec3 left_n = normalize(vec3(1.0, 0.0, -1.0));
	vec3 clamp_o = vec3(0.0, 0.0, 0.3);
	vec3 clamp_n = vec3(0.0, 0.0, 1.0);

	vec3 p = vec3((frag_coord / iResolution) - vec2(0.5), sin(iTime) * 0.2);
	float distance = max(max(
		sdPlane(p, top_n, planes_o),
		sdPlane(p, bottom_n, planes_o)),
		sdPlane(p, left_n, planes_o));
	vec3 surface_p = vec3(p.xy, distance);
	float clamp_distance = sdPlane(p, clamp_n, clamp_o);
	float cancel_term = clamp(sign(sdPlane(surface_p, clamp_n, clamp_o)), 0.0, 1.0);
	float value = step(0.0, distance);
	frag_color.xyz = vec3(57.0, 255.0, 20.0)/255.0 * distance * cancel_term * 2.0;
}