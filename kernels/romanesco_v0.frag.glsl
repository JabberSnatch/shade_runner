
mat4 perspective_inverse_hfov(float n, float f, float alpha, float aspect)
{
	float tan_half_alpha = tan(alpha * 0.5) / 1.0;
	return mat4(
		tan_half_alpha, 0.0, 0.0, 0.0,
		0.0, aspect * tan_half_alpha, 0.0, 0.0,
		0.0, 0.0, 0.0, (f-n)/(-2.0*f*n),
		0.0, 0.0, 1.0, (f+n)/(2.0*f*n)
	);
}

vec3 compute_ray_matrix(mat4 perspective_inverse, vec2 clip_coord)
{
	vec4 target = vec4(clip_coord, 1.0, 1.0);
	vec4 ray_direction = perspective_inverse * target;
	ray_direction = ray_direction / ray_direction.w;
	return normalize(ray_direction.xyz);
}

mat3 roty(float alpha)
{
    return mat3(
        cos(alpha), 0.0, sin(alpha),
        0.0, 1.0, 0.0,
        -sin(alpha), 0.0, cos(alpha)
    );
}

mat3 rotx(float alpha)
{
    return mat3(
       1.0, 0.0, 0.0,
       0.0, cos(alpha), sin(alpha),
       0.0, -sin(alpha), cos(alpha)
    );
}

float sdSphere(vec3 p, float r)
{
    return length(p) - r;
}

float sdCube(vec3 p, float r)
{
    return max(max(abs(p.x), abs(p.y)), abs(p.z)) - r;
}

float sdDiamond(vec3 p, float r)
{
    return (abs(p.x) + abs(p.y) + abs(p.z)) - r;
}

#if 1
const float kRecursionCount = 8.0;
float scene(vec3 p)
{
    vec3 o = (p - vec3(0.0, 0.0, 2.0)) * roty(iTime * 0.2) * rotx(iTime * 0.2);
    float d = 1.0 / 0.0;

    float rec = 0.0;
    float radius = pow(2.0, -rec);
    float offset = 1.0 - radius;
    vec3 cp = vec3(0.0);
    vec3 lp = o - cp*pow(2.0,-(rec-1)) + vec3(offset);
    d = min(d, sdSphere(lp, radius));
    rec += 1.0;

    for (; rec < kRecursionCount; rec+=1.0)
    {
        cp = (cp * 2.0) + max(sign(lp), 0.0);
        radius = pow(2.0, -rec);
        offset = 1.0 - radius;
        lp = (o - cp*pow(2.0,-rec+1)) + vec3(offset);
        vec3 test = cp - vec3(pow(2.0, rec));
        float yolo = 16.0;

        if (abs(test.x) < yolo || abs(test.y) < yolo || abs(test.z) < yolo)
        {
        if (rec > 6.0)
            d = max(d, -sdSphere(lp, radius));
#if 1
        else if (rec > 3.0)
             d = min(d, sdCube(lp, radius*0.8));
#endif
        else
            d = min(d, sdSphere(lp, radius));
        }
    }

    return d;
}
#endif

vec3 normal(vec3 p)
{
	float delta = 0.0001;
	return normalize(
		   vec3(scene(p + vec3(delta, 0.0, 0.0)) - scene(p - vec3(delta, 0.0, 0.0)),
		   		scene(p + vec3(0.0, delta, 0.0)) - scene(p - vec3(0.0, delta, 0.0)),
				scene(p + vec3(0.0, 0.0, delta)) - scene(p - vec3(0.0, 0.0, delta))));
}


#if 0
float scene(vec3 p)
{
    vec3 lp = (p - vec3(0.0, -0.5, 2.0)) * roty(iTime * 0.2);

    //z *= smoothstep(0.0, 1.0, lp.y) * 0.1;

    float alpha = atan(lp.z, lp.x);
    float d = (length(lp.xz) - 1.0);// - (1.0 - smoothstep(0.0, 1.0, lp.y)));
    float z = lp.y;

    float alpharep = 3.1415926536 * 0.01;
    alpha = (mod(alpha + alpharep*0.5, alpharep) - alpharep*0.5);

    float zrep = 0.05;
    float zcount = 1.0 / zrep;
    float zi = floor((z + zrep*0.5)/zrep);
    z = mod(z + zrep*0.5, zrep) - zrep*0.5;
    if (zi > zcount) z += zrep*(zi - zcount);
    if (zi < 0.0) z += zrep*zi;

    float geom =
    //scene_fract(vec3(d, alpha, z));
    sdSphere(vec3(d, alpha, z), 0.01);
    return geom;
}
#endif


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


const float kStepCount = 64.0;
const float kShadowStep = 64.0;

void imageMain(inout vec4 frag_color, vec2 frag_coord)
{
	float aspect_ratio = iResolution.y / iResolution.x;
	float fov = 90.0 * 3.1415926534 / 180.0;
	float near = 0.001;
	float far = 10000.0;
	vec2 clip_coord = ((frag_coord / iResolution) - 0.5) * 2.0;

    vec3 rd = compute_ray_matrix(perspective_inverse_hfov(near, far, fov, aspect_ratio), clip_coord);
    vec3 p = vec3(0.);
    float i = 0.;
    for (; i < kStepCount; i+=1.0)
    {
        float d = scene(p);
        p += rd * d;
        if (d < 0.001) break;
    }

    vec3 v = -rd;
    vec3 n = normal(p);
    vec3 lp = vec3(3.0, 0.0, 2.0);
    vec3 l = normalize(lp-p);
    vec3 h = normalize(l+v);
 	float F = fresnel_term(l, n, 1.0, 2.0);

    vec3 sp = lp;
    int srm_step = 0;
    float srm_dist = scene(sp);
	for (; srm_step < kShadowStep; srm_step++)
	{
		sp -= l * srm_dist;
        srm_dist = scene(sp);
        if (srm_dist < 0.005)
           break;
   	}
    float occlusion = step(distance(p, sp), 0.1);
    frag_color.xyz = dot(n, l) * vec3(0.0, 0.2, 0.7) * (1.0+F) * occlusion * 2.0;
    frag_color.xyz += vec3(1.0, 1.0, 1.0) * pow(max(0.0, dot(n, h)), 10.0) * occlusion;

    lp = vec3(-2.0, 1.0, 0.0);
    l = normalize(lp-p);
    h = normalize(l+v);
 	F = fresnel_term(l, n, 1.0, 2.0);
    sp = lp;
    srm_step = 0;
    srm_dist = scene(sp);
	for (; srm_step < kShadowStep; srm_step++)
	{
		sp -= l * srm_dist;
        srm_dist = scene(sp);
        if (srm_dist < 0.005)
           break;
   	}
    occlusion = step(distance(p, sp), 0.1);
    frag_color.xyz += dot(n, l) * vec3(0.8, 0.3, 0.1) * (1.0+F) * occlusion;
    frag_color.xyz += vec3(0.5, 0.5, 0.0) * pow(max(0.0, dot(n, h)), 10.0) * occlusion;

    return;
    float test = 1.0 - (i / kStepCount);
    frag_color.xyz *= test*test;
}
