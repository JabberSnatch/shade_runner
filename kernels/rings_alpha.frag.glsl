
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
	float angle = (atan(p.x, p.y) / kPi) * 0.5 + 0.5;
	float distance = length(p) - iTime * 0.5;
	distance *= 4.0;
	float radius = floor(distance);
	angle *= 3.0;
	float rotation_offset = .5;
	angle += iTime * mix(-rotation_offset, rotation_offset, rand(vec2(radius, 0.2)));

	float index = floor(distance);

	frag_color = vec4(step(0.1, fract(distance)) * step(-0.9, -fract(distance)) * step(0.5, fract(angle)));
}