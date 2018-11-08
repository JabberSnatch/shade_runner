// ================================================================================
// PRIMITIVES
// ================================================================================

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

float sdSphere(vec3 p, float r)
{
	return length(p) - r;
}

vec3 sdTruncatedCylinder_normal(vec3 p, float r, float h)
{
	float cylinder = sdCylinder(p, r);
	float bottom = sdPlane(p, vec3(0.0, -1.0, 0.0), vec3(0.0, h/2.0, 0.0));
	float top = sdPlane(p, vec3(0.0, 1.0, 0.0), vec3(0.0, -h/2.0, 0.0));
	float cylinder_closer_bottom = step(cylinder, bottom);
	float bottom_closer_top = step(bottom, top);
	float top_closer_cylinder = step(top, cylinder);
	// cylinder -> A && !C
	// bottom -> B && !A
	// top -> C && !B
	return vec3(0.0);
}

float scene(vec3 p)
{
	float A = sdTruncatedCylinder(p - vec3(cos(iTime) * 1.0, sin(iTime) * -1.5, 5.0), 1.0, (cos(iTime * 2.0) + 1.0) * 1.0 + 0.1);

	float B = sdSphere(p - vec3(cos(iTime), 0.0, sin(iTime)) * 7.0, 1.0);

	return min(A, B);
}

vec3 normal(vec3 p)
{
	return vec3(0.0);
}

// ================================================================================
// RAYCASTING
// ================================================================================

vec3 target_half_diagonal_hfov(float n, float alpha, float aspect)
{
	float half_width = tan(alpha * 0.5) * n;
	float half_height = half_width * aspect;
	return vec3(half_width, half_height, n);
}

vec3 compute_ray_plane(vec3 half_diagonal, vec2 clip_coord)
{
	vec3 target = half_diagonal * vec3(clip_coord, 1.0);
	return normalize(target);
}


// ================================================================================
// MAIN
// ================================================================================

const int kMaxStep = 200;
const float kDistanceThreshold = 0.01;

void imageMain(inout vec4 frag_color, vec2 frag_coord)
{
	float aspect_ratio = iResolution.y / iResolution.x;
	float fov = 90.0 * 3.1415926535 / 180.0;
	float near = 0.1;
	vec3 half_diagonal = target_half_diagonal_hfov(near, fov, aspect_ratio);
	vec2 clip_coord = ((frag_coord / iResolution) - 0.5) * 2.0;
	vec3 ray = compute_ray_plane(half_diagonal, clip_coord);

	vec3 position = vec3(0.0);
	float distance = scene(position);
	int rm_step = 0;
	bool hit = false;
	while (distance > kDistanceThreshold && rm_step < kMaxStep)
	{
		position += ray * distance * 0.9;
		distance = scene(position);
		rm_step++;
	}

	hit = distance <= kDistanceThreshold;

	vec3 bg_color = vec3(0.2, 0.2, 0.2);
#if 0
	frag_color.xyz = mix(
				   position, bg_color,
				   step(kDistanceThreshold, distance));
	frag_color.xyz = mix(
				   bg_color, position,
				   step(distance, kDistanceThreshold));
#endif
	if (hit)
	{
		frag_color.xyz = position;
	}
	else
	{
		frag_color.xyz = bg_color;
	}

	//frag_color.xyz = mix(vec3(0.5, 0.5, 0.5), frag_color.xyz, step(float(rm_step), float(kMaxStep)));
}