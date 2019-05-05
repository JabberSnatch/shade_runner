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
	return max(
		sdCylinder(p, r),
		max(
		sdPlane(p, vec3(0.0, -1.0, 0.0), vec3(0.0, h/2.0, 0.0)),
		sdPlane(p, vec3(0.0, 1.0, 0.0), vec3(0.0, -h/2.0, 0.0))));
}

float sdCube(vec3 p, float side)
{
	vec3 rm_dist = abs(p) - vec3(side * 0.5);
	vec3 sqrRmDist = rm_dist * rm_dist;
	return max(rm_dist.x, max(rm_dist.y, rm_dist.z)) -
		   sqrt(abs(sqrRmDist.x + sqrRmDist.y + sqrRmDist.z)) * 0.5;
}

float sdOctahedron(vec3 p, float side)
{
	vec3 rm_dist = abs(p) - vec3(side * 0.5);
	return (rm_dist.x + rm_dist.y + rm_dist.z) / 3.0;
}

float sdSphere(vec3 p, float r)
{
	return length(p) - r;
}

float scene(vec3 p)
{
	float stride = 3.0;
	p.x = mod(p.x + stride, stride*2.0) - stride;
	p.y = mod(p.y + stride, stride*2.0) - stride;

	float time = iTime * 10.0;
	float x_angle = mod(time, 360.0) * PI / 180.0;
	float y_angle = mod(time, 360.0) * PI / 180.0;
	mat3 local_space = inverse(rotateX(x_angle) * rotateY(y_angle));
	mat3 static_rotation = rotateX(PI / 4.0) * rotateY(PI / 4.0);

	return min(sdCube(local_space * (p - vec3(0.0, 0.0, 5.0)), 2.0),
		   	   sdPlane(p, vec3(0.0, 0.0, 1.0), vec3(0.0, 0.0, 15.0)));
}

vec3 scene_color(vec3 p)
{
	return vec3(0.2, 0.3, 0.5) * 2.0;
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

const int kMaxStep = 64;
const int kShadowStep = 32;
const float kStepMultiplier = 1.0;

void imageMain(inout vec4 frag_color, vec2 frag_coord)
{
	float aspect_ratio = iResolution.y / iResolution.x;
	float fov = 60.0 * PI / 180.0;
	float near = 0.1;
	vec3 half_diagonal = target_half_diagonal_hfov(near, fov, aspect_ratio);
	vec2 clip_coord = ((frag_coord / iResolution) - 0.5) * 2.0;
	vec3 ray = compute_ray_plane(half_diagonal, clip_coord);

	vec3 position = vec3(0.0, 0.0, -50.0);
	float rm_dist = scene(position);

	for (int rm_step = 0; rm_step < kMaxStep; rm_step++)
	{
		position += ray * rm_dist * kStepMultiplier;
		rm_dist = scene(position);
	}

	vec3 bg_color = vec3(0.2);

	float light_falloff = 5.0;
	vec3 lp = vec3(sin(iTime) * 3.0, cos(iTime) * 3.0, 2.0);

	vec3 n = normal(position);
	vec3 l = normalize(lp - position);
	float Li = dot(n, l) / (distance(lp, position) / light_falloff);
	float F = fresnel_term(l, n, 1.0, 1.5);
	vec3 base_color = scene_color(position);

	vec3 sp = lp;
	for (int shadow_step = 0; shadow_step < kShadowStep; shadow_step++)
	{
		sp -= l * scene(sp);
   	}
	float occlusion = clamp(1.0 - distance(position, sp), 0.0, 1.0);

	frag_color.xyz = base_color * (Li * (1.0 + F)) * occlusion;
}