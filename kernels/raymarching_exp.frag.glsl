
const float pi = 3.1415926535;

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

mat4 perspective_hfov(float n, float f, float alpha, float aspect)
{
	float inverse_tan_half_alpha = 1.0 / tan(alpha * 0.5);
	return mat4(
		inverse_tan_half_alpha, 0.0, 0.0, 0.0,
		0.0, (1.0/aspect)*inverse_tan_half_alpha, 0.0, 0.0,
		0.0, 0.0, (-(f+n))/(f-n), (-2.0*f*n)/(f-n),
		0.0, 0.0, -1.0, 0.0
	);
}

vec3 target_half_diagonal_hfov(float n, float alpha, float aspect)
{
	float half_width = tan(alpha * 0.5) * n;
	float half_height = half_width * aspect;
	return vec3(half_width, half_height, n);
}

vec3 compute_ray_matrix(mat4 perspective_inverse, vec2 clip_coord)
{
	vec4 target = vec4(clip_coord, 1.0, 1.0);
	vec4 ray_direction = perspective_inverse * target;
	ray_direction = ray_direction / ray_direction.w;
	return normalize(ray_direction.xyz);
}

vec3 compute_ray_plane(vec3 half_diagonal, vec2 clip_coord)
{
	vec3 target = half_diagonal * vec3(clip_coord, 1.0);
	return normalize(target);
}

void imageMain(inout vec4 frag_color, vec2 frag_coord)
{
	float aspect_ratio = iResolution.y / iResolution.x;
	float fov = 90.0 * pi / 180.0;
	float near = 0.001;
	float far = 10000.0;
	vec2 clip_coord = ((frag_coord / iResolution) - 0.5) * 2.0;

	vec3 half_diagonal = target_half_diagonal_hfov(near, fov, aspect_ratio);

#if 1
	mat4 perspective_inverse = perspective_inverse_hfov(near, far, fov, aspect_ratio);
#else
	mat4 perspective_inverse = inverse(perspective_hfov(near, far, fov, aspect_ratio));
#endif


#if 1
// ================================================================================
// MATRIX
// ================================================================================

	frag_color.xyz = abs(compute_ray_matrix(perspective_inverse, clip_coord));

#if 1
	vec4 target = vec4(clip_coord, 1.0, 1.0);
	vec4 ray_direction = perspective_inverse * target;
    ray_direction = ray_direction / ray_direction.w;
	frag_color.xyz = (normalize(ray_direction.xyz));
#endif

	vec3 side1 = compute_ray_matrix(perspective_inverse, vec2(-1.0, 0.0));
	vec3 side2 = compute_ray_matrix(perspective_inverse, vec2(1.0, 0.0));

#else
// ================================================================================
// PLANE (baseline)
// ================================================================================

	frag_color.xyz = (compute_ray_plane(half_diagonal, clip_coord));

	vec3 side1 = compute_ray_plane(half_diagonal, vec2(-1.0, 0.0));
	vec3 side2 = compute_ray_plane(half_diagonal, vec2(1.0, 0.0));
#endif

#if 0
	vec3 matrix_ray = compute_ray_matrix(perspective_inverse, clip_coord);
	vec3 plane_ray = compute_ray_plane(half_diagonal, clip_coord);
	frag_color.xyz = abs(plane_ray - matrix_ray);
#endif

#if 0
	float error = abs(dot(side1, side2) - cos(fov));
	if (error <= 0.000001)
		frag_color.xyz = vec3(1.0);
	else
		frag_color = frag_color;
		//frag_color.xyz = vec3(error);
#endif
}