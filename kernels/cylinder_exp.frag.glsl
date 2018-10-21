
float sdCylinder(vec3 p, float r)
{
	return length(p.xz) - r;
}

float sdPlane(vec3 p, vec3 n, vec3 o)
{
	return dot(o - p, n);
}

float sdTruncatedCylinder(vec3 p, float r, float h)
{
	return max(
		sdCylinder(p, r),
		max(
		sdPlane(p, vec3(0.0, -1.0, 0.0), vec3(0.0, h/2.0, 0.0)),
		sdPlane(p, vec3(0.0, 1.0, 0.0), vec3(0.0, -h/2.0, 0.0))));
}

void imageMain(inout vec4 frag_color, vec2 frag_coord)
{
	vec3 p = vec3((((frag_coord / iResolution) - 0.5) * 5.0), sin(iTime));

	frag_color.y = -sdTruncatedCylinder(p, 1.5, 5.0);
}