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

vec3 compute_ray_matrix(mat4 perspective_inverse, vec2 clip_coord)
{
	vec4 target = vec4(clip_coord, 1.0, 1.0);
	vec4 ray_direction = perspective_inverse * target;
	ray_direction = ray_direction / ray_direction.w;
	return normalize(ray_direction.xyz);
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
const float kStepMultiplier = 1.0;

float GDF_cube(vec3 p, float radius, float exp)
{
    vec3 ni = abs(p);
	return pow(pow(ni.x, exp) + pow(ni.y, exp) + pow(ni.z, exp), 1.0/exp) - radius;
}

float GDF_ico(vec3 p, float radius, float exp)
{
    return pow(
        pow(abs(dot(p, vec3(0.577))), exp) +
        pow(abs(dot(p, vec3(-1.0, 1.0, 1.0) * 0.577)), exp) +
        pow(abs(dot(p, vec3(1.0, -1.0, 1.0) * 0.577)), exp) +
        pow(abs(dot(p, vec3(1.0, 1.0, -1.0) * 0.577)), exp) +
        pow(abs(dot(p, vec3(0.0, 0.357, 0.934))), exp) +
        pow(abs(dot(p, vec3(0.0, -0.357, 0.934))), exp) +
        pow(abs(dot(p, vec3(0.934, 0.0, 0.357))), exp) +
        pow(abs(dot(p, vec3(-0.934, 0.0, 0.357))), exp) +
        pow(abs(dot(p, vec3(0.357, 0.934, 0.0))), exp) +
        pow(abs(dot(p, vec3(-0.357, 0.934, 0.0))), exp)
        , 1.0 / exp) - radius;
}

float smooth_min(float a, float b, float e)
{
    float res = exp(-e*a) + exp(-e*b);
    return -log(max(0.0001, res)) / e;
}

float scene(vec3 p)
{
	return smooth_min(GDF_ico(p - iGizmos[0], 1.0, 32.0),
                      GDF_ico(rotateX(3.1415926536 * 0.25) * (p - vec3(sin(iTime), cos(iTime), -5.0)), 1.0, 32.0),
                      2.0);
}

vec3 normal(vec3 p)
{
	float delta = 0.0005;
	return normalize(
		   vec3(scene(p + vec3(delta, 0.0, 0.0)) - scene(p - vec3(delta, 0.0, 0.0)),
		   		scene(p + vec3(0.0, delta, 0.0)) - scene(p - vec3(0.0, delta, 0.0)),
				scene(p + vec3(0.0, 0.0, delta)) - scene(p - vec3(0.0, 0.0, delta))));
}


vec3 scene_color(vec3 p)
{
	return vec3(0.4, 0.8, 0.5) * 1.0;
}

void imageMain(inout vec4 frag_color, vec2 frag_coord)
{
	float aspect_ratio = iResolution.y / iResolution.x;
	float fov = 60.0 * PI / 180.0;

	float near = 0.1;
	vec3 half_diagonal = target_half_diagonal_hfov(near, fov, aspect_ratio);
	vec2 clip_coord = ((frag_coord / iResolution) - 0.5) * 2.0;
	vec3 ray = compute_ray_matrix(inverse(iProjMat), clip_coord);
    ray = rotateX(-sin(iTime) * PI / 64) * ray;

	vec3 position = vec3(0.0, 0.0, 0.0);//sin(iTime * 0.2), cos(iTime * 0.2), -5.0);
    vec3 start_pos = position;
	float rm_dist = scene(position);
    float min_dist = 1.0/0.0;

    int rm_step = 0;
	for (; rm_step < kMaxStep; rm_step++)
	{
		position += ray * rm_dist * kStepMultiplier;
		rm_dist = scene(position);
        min_dist = min(min_dist, rm_dist);
        if (rm_dist < 0.005)
           break;
	}

	vec3 bg_color = vec3(0.2);
    //bg_color = vec3(1.0) * fract(min_dist);

	float light_intensity = 0.5;
	vec3 lp = vec3(-1.0, 3.0, 3.0);

	vec3 n = normal(position);
	vec3 l = vec3(1.0, 3.0, 3.0);
	float Li = dot(n, l) * light_intensity / distance(lp, position);
	float F = fresnel_term(l, n, 1.0, 2.0);
	vec3 base_color = scene_color(position);

    if (rm_dist < 0.005)
        frag_color.xyz = base_color * (Li * (1.0 + F));
    else
        frag_color.xyz = bg_color;
}
