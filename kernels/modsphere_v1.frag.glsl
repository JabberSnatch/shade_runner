#define PI 3.1415926535

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

float sdSphere(vec3 pos, float radius)
{
	return length(pos) - radius;
}

mat2 rot2(float theta)
{
	return mat2(
		   cos(theta), sin(theta),
		   -sin(theta), cos(theta)
		   );
}

float scene(vec3 in_pos)
{
	//pos -= vec3(0.0, 0.0, 1.0);
	//return sdSphere(pos, 0.2);
	float dist = 65535.0;
	for (float i = -3.0; i <= 3.0; i+=1.0)
	{
		vec3 pos = in_pos;
		pos.x += ((step(abs(i), 1.0) * 2.0) - 1.0) * i * 0.09;
		pos.y += i * 0.05 * (sin(iTime) + sin(iTime * 3.0) * 0.5 + sin(iTime * 15.0) * 0.1);
		pos.z += mod(pos.z - 0.5, 0.2 * (sin(iTime * 0.3) * 0.25 + 1.5));
		//pos.x = mod(pos.x, 0.1 * sin(iTime));
		//pos.y = mod(pos.y, 0.2 * (sin(gl_FragCoord.x * 2.0 / iResolution.x) + sin(iTime * 0.2)));
		//pos.xz = rot2(iTime) * pos.xz;
		//pos.yz = rot2(iTime * 0.1) * pos.yz;
		dist = min(dist, sdSphere(pos - vec3(0.0, 0.0, 1.0), 0.05));//vec3(x, pos.yz), 0.2));
	}
	for (float i = -2.0; i <= 2.0; i+=1.0)
	{
		vec3 pos = in_pos;
		pos.z -= (sin(iTime) * 0.5 + 0.5) * 0.5;
		pos.y += ((step(abs(i), 1.0) * 2.0) - 1.0) * i * 0.09;
		pos.x -= i * 0.05 * (sin(iTime) + sin(iTime * 3.0) * 0.5 + sin(iTime * 15.0) * 0.1);
		pos.z += mod(pos.z - 0.5, 0.2 * (sin(iTime * 0.3) * 0.25 + 1.5));
		dist = min(dist, sdSphere(pos - vec3(0.0, 0.0, 1.0), 0.05));//vec3(x, pos.yz), 0.2));
	}
	return dist;
}


const float kMaxStep = 10.0;
const float kStep = 1.0;
const float kDistanceThreshold = 0.001;

void imageMain(inout vec4 frag_color, vec2 frag_coord)
{
	float aspect_ratio = iResolution.y / iResolution.x;
	float fov = 60.0 * PI / 180.0;
	float near = 0.1;
	vec3 half_diagonal = target_half_diagonal_hfov(near, fov, aspect_ratio);
	vec2 clip_coord = ((frag_coord / iResolution) - 0.5) * 2.0;
	vec3 ray = compute_ray_plane(half_diagonal, clip_coord);

	vec3 position = vec3(0.0);
	float distance = kDistanceThreshold * 2.0;
	float min_dist = 2.0;
	float rm_step = 0.0;
	bool hit = false;
	for(; distance > kDistanceThreshold && rm_step < kMaxStep; rm_step += kStep)
	{
		distance = scene(position);
		position += ray * distance;
		min_dist = min(distance, min_dist);
	}

	//float shade = smoothstep(0.0, kMaxStep, rm_step);
	//float shade = (rm_step - 1.0) / kMaxStep;
	float shade = 0.0;
	if (rm_step < kMaxStep)
	{
		shade = exp((rm_step - 1.0) / kMaxStep);
	}
	if (rm_step >= kMaxStep)
	{
		shade = smoothstep(0.05, -0.05, min_dist);
	}

	vec2 uv = vec2(clip_coord.x, clip_coord.y * aspect_ratio);
	float uv_dist = length(uv);
	float inner = 0.13;
	float outer = 0.15;
	shade = mix(shade, 1.0 - shade, min(step(uv_dist, outer), step(inner, uv_dist)));

	frag_color = vec4(vec3(shade), 1.0);
}