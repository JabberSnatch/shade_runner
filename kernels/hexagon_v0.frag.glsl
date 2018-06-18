
const float kPi = 3.1415926535;
const float one_third = kPi/3.;
const float one_sixth = kPi/6.;

vec2 aspect_factor = vec2(iResolution.x / iResolution.y, 1.0);

vec2 NDC(vec2 frag_coord)
{
	return (frag_coord / iResolution) * 2.0 - 1.0;
}

vec2 VerticalAspectCoordinates(vec2 frag_coord)
{
	vec2 ndc = NDC(frag_coord);
	return ndc * aspect_factor;
}

vec2 InverseVerticalAspect(vec2 aspect_coord)
{
	return ((aspect_coord / aspect_factor) + 1.0) * 0.5 * iResolution;
}

float sdHexagon(vec2 p, float hex_radius)
{
	float alpha = atan(p.x, p.y);
	float beta = mod(alpha, one_third);
	float gamma = 4.0 * one_sixth - beta;
	float distance = (sin(gamma) * length(p)) / sin(2.0 * one_sixth);
	return distance - hex_radius;
}

float sdHexagon_alt(vec2 p, float hex_radius)
{
	float alpha = atan(p.x, p.y);
	float beta = mod(alpha, one_third) - one_sixth;// * (cos(iTime) + 1.0);
	float threshold = (-log(cos(beta)) + cos(one_sixth)) * hex_radius;
	float distance = length(p) / (-log(cos(beta)) + cos(one_sixth));
	return distance - hex_radius;
}

void imageMain(inout vec4 frag_color, vec2 frag_coord)
{
	float scale = 1.0;
	float hex_radius = 0.5;
	float pixel_size = scale * (2.0 / iResolution.y);

	vec2 p = VerticalAspectCoordinates(frag_coord) * scale;
	frag_color = vec4(fract(p + vec2(0.5)), 0.0, 1.0);

	vec3 background_color = vec3(1.0, 1.0, 0.0);
	vec3 hexagon_color = vec3(0.0, 0.0, 1.0);

// SDF AA (or close enough)
	float blur_radius = 2.0 * pixel_size;
	float distance = sdHexagon(p, hex_radius);
	float shape_alpha = clamp(-distance / blur_radius, 0.0, 1.0);

	frag_color.xyz = mix(background_color, hexagon_color, shape_alpha);
	frag_color.w = 1.0;
}