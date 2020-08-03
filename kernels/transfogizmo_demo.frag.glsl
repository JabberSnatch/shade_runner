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

float sdSphere(vec3 p, float r)
{
	return length(p) - r;
}

float sdCube(vec3 p, float r)
{
    vec3 ap = abs(p) - r;
    return max(ap.x, max(ap.y, ap.z));
}

float sdDiamond(vec3 p, float r)
{
    return ((abs(p.x) + abs(p.y) + abs(p.z)) - r) / sqrt(3.0);
}

float scene(vec3 p)
{
    float distance = sdSphere(p - iGizmos[0], 10.0);
    for (int i = 1; i < iGizmoCount-1; ++i)
    {
        float lhs = 0.0;
#if 1
        if (i % 2 == 0)
            lhs = sdSphere(p - iGizmos[i], 1.0);
        else
            lhs = sdDiamond(p - iGizmos[i], 1.0);
#else
        lhs = sdSphere(p - iGizmos[i], 1.0);
#endif

        float rhs = distance;
        vec2 muc = max(vec2(0.1 - lhs, 0.1 - rhs), vec2(0.0));
        distance = max(0.1, min(lhs, rhs)) - length(muc);
    }
    return distance;
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

const int kMaxStep = 8;
const int kShadowStep = 8;
const float kStepMultiplier = 1.0;

void imageMain(inout vec4 frag_color, vec2 frag_coord)
{
	vec2 clip_coord = ((frag_coord / iResolution) - 0.5) * 2.0;
    vec3 ray = compute_ray_matrix(inverse(iProjMat), clip_coord);

    vec4 origin = inverse(iProjMat) * vec4(0.0, 0.0, -1.0, 1.0);
	vec3 position = origin.xyz / origin.w;
	float rm_dist = scene(position);

	for (int rm_step = 0; rm_step < kMaxStep; rm_step++)
	{
		position += ray * rm_dist * kStepMultiplier;
		rm_dist = scene(position);
	}

	vec3 bg_color = vec3(0.2);

	float light_falloff = 10.0;
	vec3 lp = iGizmos[iGizmoCount-1];//vec3(sin(iTime) * 5.0, cos(iTime) * 3.0, 2.0);

	vec3 n = normal(position);
	vec3 l = normalize(lp - position);
	float Li = dot(n, l) / (distance(lp, position) / light_falloff);
	float F = fresnel_term(l, n, 1.0, 1.5);
	vec3 base_color = scene_color(position);

#if 1
	vec3 sp = lp;
	for (int shadow_step = 0; shadow_step < kShadowStep; shadow_step++)
	{
		sp -= l * scene(sp);
   	}
	float occlusion = clamp(1.0 - distance(position, sp), 0.0, 1.0);
#else
    float occlusion = 1.0;
#endif

	frag_color.xyz = base_color * (Li * (1.0 + F)) * occlusion;
	if (rm_dist > 0.1)
	{
        float pt = (-100.f - origin.y) / ray.y;
        if (pt > 0.f)
        {
            vec3 p = (origin.xyz/origin.w) + ray * pt;
            frag_color.xyz = mix(vec3(0.7, 0.5, 0.1), vec3(0.5, 0.2, 0.4), sin(length(p*0.01)) * 0.5 + 0.5) * 0.2;
        }
        else
            frag_color.xyz = bg_color;//vec3(0.0, 0.0, 0.0);
	}
}
