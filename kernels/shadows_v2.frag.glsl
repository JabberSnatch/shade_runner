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

float sdCube(vec3 p, float side)
{
	vec3 rm_dist = abs(p) - vec3(side * 0.5);
	vec3 sqrRmDist = rm_dist * rm_dist;
	return max(rm_dist.x, max(rm_dist.y, rm_dist.z)) -
		   sqrt(abs(sqrRmDist.x + sqrRmDist.y + sqrRmDist.z)) * 0.5;//(cos(iTime * 2.0) + 1.0) * 0.3;
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

#define SHADOW_RAYS
const int kMaxStep = 128;
const int kShadowStep = 64;
const float kStepMultiplier = 1.0;
const float kBpm = 120.0;
#define PULSE(bpm, signature, release) (fract(iTime * bpm / 60.0 / signature) < release)
#define SAWTOOTH(bpm, signature, release)


float pulse(float time, float bpm, float signature)
{
	return fract((time * bpm) / (60.0 * signature));
}

float cycle(float time, float bpm, float signature, float release)
{
	float pulse = pulse(time, bpm, signature);
	return step(pulse, release) * (pulse / release);
}

float envelope(float t, float attack, float decay, float sustain)
{
	float attack_segment = (t / attack) * step(t, attack);
	float decay_segment = mix(1.0, sustain, (t-attack)/decay) * step(t-attack, decay) * step(attack, t);
	float sustain_segment = sustain * step(decay, t-attack);
	return attack_segment + decay_segment + sustain_segment;
}


const float kBar = 30.0;
const float kSignature = 4.0;

bool i_glitchFrame()
{
	return
		PULSE(120.0, 2.0, 0.5);
	return
		floor(mod(iTime * 6.0, 3.0)) <= 1.0 &&
     	floor(mod(iTime * 0.5, 2.0)) == 0.0 &&
       	floor(mod(iTime * 0.8, 2.0)) == 0.0;
}


float scene(vec3 p)
{
	float stride = 3.0;
    vec3 cp = vec3(3.0, 3.0, 5.0);
	p.x = mod(p.x - cp.x + stride, stride*2.0) - stride + cp.x;
	p.y = mod(p.y - cp.y + stride, stride*2.0) - stride + cp.y;
	p.z = mod(max(p.z, 0.0), stride*4.0 + 20.0);//(sin(iTime) + 1.0) * 10.0);

	float time = (iTime - 50.0 * envelope(cycle(iTime, kBar, 3.0, 0.33333), 0.1, 0.9, 0.0)) * 10.0;
#if 0
	float time = iTime * (1.0 + envelope(cycle(iTime, kBar, 3.0, 0.33333 * 0.5), 0.1, 0.3, 0.7));
#if 0
	time *= envelope(cycle(iTime, kBar, 3.0, 0.33333 * 0.5), 0.1, 0.3, 0.7);
#else
	if (PULSE(kBar, 3.0, 0.33333 * 0.5))
	   time *= 50.0;
#endif
#endif
	float x_angle = mod(time, 360.0) * PI / 180.0;
	float y_angle = mod(time, 360.0) * PI / 180.0;
	mat3 local_space = inverse(rotateX(x_angle) * rotateY(y_angle));
	mat3 static_rotation = rotateX(PI / 4.0) * rotateY(PI / 4.0);

	return sdCube(local_space * (p - cp), 2.0);
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
	return vec3(0.2, 0.3, 0.5) * 2.0;
}

void imageMain(inout vec4 frag_color, vec2 frag_coord)
{
	// Screen glitches
	// Screen shake
	// Shape
	// Amount
	// Render mode
	// FOV

	if (PULSE(kBar, 4.0, 0.5))
	{
		if (mod(frag_coord.y, 20.0) < 2.0)
		   frag_coord.x += sin(iTime * 50.0) * 5.0;
	}

	float aspect_ratio = iResolution.y / iResolution.x;
	float fov = 60.0 * PI / 180.0;

	float near = 0.1;
	vec3 half_diagonal = target_half_diagonal_hfov(near, fov, aspect_ratio);
	vec2 clip_coord = ((frag_coord / iResolution) - 0.5) * 2.0;
	vec3 ray = compute_ray_plane(half_diagonal, clip_coord);
    ray = rotateX(-sin(iTime) * PI / 64) * ray;

	vec3 position = vec3(cos(iTime), 0.0, -20.0);
    vec3 start_pos = position;
	float rm_dist = scene(position);

    int rm_step = 0;
	for (; rm_step < kMaxStep; rm_step++)
	{
		position += ray * rm_dist * kStepMultiplier;
		rm_dist = scene(position);
        if (rm_dist < 0.005)
           break;
	}

	vec3 bg_color = vec3(0.2);

	float light_intensity = 5.0;
	vec3 lp = vec3(sin(iTime) * 7.0, cos(iTime) * 7.0, 3.0);

	vec3 n = normal(position);
	vec3 l = normalize(lp - position);
	float Li = dot(n, l) * light_intensity / distance(lp, position);
	float F = fresnel_term(l, n, 1.0, 2.0);
	vec3 base_color = scene_color(position);

#ifdef SHADOW_RAYS
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
	float occlusion = clamp(1.0 - distance(position, sp), 0.0, 1.0);
    occlusion += 1.0 - step(srm_dist, 0.1);
    occlusion = min(occlusion, 1.0);
#else
    float occlusion = 1.0;
#endif

	frag_color.xyz = base_color * (Li * (1.0 + F)) * occlusion;
	if (true || PULSE(kBar, 1.0, 0.5))
	//&& PULSE(kBar, 0.33333, 0.5))
	{
        frag_color.xyz = vec3((float(rm_step) / float(kMaxStep)));
    }
	//frag_color.xyz = vec3(envelope(cycle(iTime, kBar, 3.0, 0.33333 * 0.5), 0.1, 0.3, 0.7));
}