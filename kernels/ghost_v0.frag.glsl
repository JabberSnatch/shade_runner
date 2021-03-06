// ================================================================================
// MATHS
// ================================================================================

#define PI 3.1415926535

mat3 rotateX(float alpha)
{
	return mat3(
		1.0, 0.0, 0.0,
		0.0, cos(alpha), sin(alpha),
		0.0, -sin(alpha), cos(alpha)
	);
}

mat3 rotateY(float alpha)
{
	return mat3(
		cos(alpha), 0.0, -sin(alpha),
		0.0, 1.0, 0.0,
		sin(alpha), 0.0, cos(alpha)
	);
}

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
	p = abs(p) - vec3(h*0.5);
	return max(p.x, max(p.y, p.z));
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

vec3 cylinder_position(vec3 p)
{
	float time = iTime * 0.5;
	float x_angle = mod(time * 90.0, 360.0) * PI / 180.0;
	float y_angle = mod(time * 72.0, 360.0) * PI / 180.0;
	vec3 cylinder_local_pos = (rotateX(x_angle) * rotateY(y_angle) * (p - vec3(0.0, 0.0, 5.0)));
	return cylinder_local_pos;
}

vec3 sphere_position(vec3 p)
{
	return p - vec3(cos(iTime), 0.0, abs(sin(iTime))) * 7.0;
}

float scene(vec3 p)
{
#if 1
	p.x = mod(p.x + 2.5, 5.0) - 2.5;
	p.y = mod(p.y + 2.5, 5.0) - 2.5;
	//p = p * 0.5;
#endif
	float A = sdTruncatedCylinder(cylinder_position(p), 1.0, 2.0);
	return A;
	float B = sdSphere(sphere_position(p), 1.0);
	return min(A, B);
}

vec3 scene_color(vec3 p)
{
	return vec3(0.2, 0.3, 0.5);
	vec3 cyl_pos = cylinder_position(p);
	float angle = atan(cyl_pos.x, cyl_pos.z);
	float red_step = step(0.1 * PI, mod(angle, 0.2 * PI));
	vec3 cylinder_color = red_step * vec3(0.97, 0.28, 0.09) + (1.0 - red_step) * vec3(1.0);

	return cylinder_color;
}

vec3 normal(vec3 p)
{
	float delta = 0.0001;
	return normalize(
		   vec3(scene(p + vec3(delta, 0.0, 0.0)) - scene(p - vec3(delta, 0.0, 0.0)),
		   		scene(p + vec3(0.0, delta, 0.0)) - scene(p - vec3(0.0, delta, 0.0)),
				scene(p + vec3(0.0, 0.0, delta)) - scene(p - vec3(0.0, 0.0, delta))));
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
// LIGHTING
// ================================================================================

float fresnel_term(vec3 l, vec3 n, float ior_out, float ior_in)
{
	float c = abs(dot(-l, n));
	float g_squared = ((ior_in * ior_in) / (ior_out * ior_out)) - 1.0 + (c * c);
	if (g_squared < 0.0) return 1.0;
	float g = sqrt(g_squared);
	float gmc = g - c;
	float gpc = g + c;
	float T0 = c*gpc - 1.0;
	float T1 = c*gmc + 1.0;
	float result = 0.5 * ((gmc * gmc)/(gpc * gpc)) * (1.0 + ((T0 * T0) / (T1 * T1)));
	return result;
}

// ================================================================================
// MAIN
// ================================================================================

const int kMaxStep = 10;
const float kDistanceThreshold = 0.01;

void imageMain(inout vec4 frag_color, vec2 frag_coord)
{
	float aspect_ratio = iResolution.y / iResolution.x;
	float fov = 90.0 * PI / 180.0;
	float near = 0.1;
	vec3 half_diagonal = target_half_diagonal_hfov(near, fov, aspect_ratio);
	vec2 clip_coord = ((frag_coord / iResolution) - 0.5) * 2.0;
	vec3 ray = compute_ray_plane(half_diagonal, clip_coord);

	vec3 position = vec3(0.0, 0.0, -3.0);
	float distance = scene(position);
	int rm_step = 0;
	bool hit = false;
	while (distance > kDistanceThreshold && rm_step < kMaxStep)
	{
		position += ray * distance;
		distance = scene(position);
		rm_step++;
	}

	hit = distance <= kDistanceThreshold;

	vec3 bg_color = vec3(0.2);
#if 0
	float Li = dot(normal(position), normalize(vec3(0.0, 0.5, -0.5)));
	vec3 color = Li * scene_color(position);

	frag_color.xyz = mix(
// NOTE: ???????
// Changing x seems to change y even though they aren't related. How is this possible
#if 1
		color,
#else
		vec3(0.0, 1.0, 0.0),
#endif
		bg_color,
		step(kDistanceThreshold, distance));
#else
	if (hit)
	{
		vec3 lp = vec3(1.0, 0.0, 0.0);

		vec3 n = normal(position);
		//vec3 l = normalize(vec3(cos(iTime) * 1.0, 0.5, sin(iTime) * 1.0));
		vec3 l = normalize(lp - position);
		float Li = 1.0;//dot(n, l);
		float F = fresnel_term(l, n, 1.0, 1.5);
		vec3 base_color = scene_color(position);
		frag_color.xyz = base_color * (Li * (1.0 + F));
		//frag_color.xyz = vec3(float(rm_step) / kMaxStep);
		//frag_color.xyz = vec3(distance);
	}
	else
	{
		frag_color.xyz = bg_color;
	}
	frag_color.xyz = vec3(1.0 - distance);
	//frag_color.xyz = vec3(float(rm_step) / kMaxStep);
#endif

	//frag_color.xyz = mix(vec3(0.5, 0.5, 0.5), frag_color.xyz, step(float(rm_step), float(kMaxStep)));
}