vec3 hash3(vec3 p)
{
    mat3 seed = mat3(742.342, 823.457, 242.086,
                     247.999, 530.343, 634.112,
                     437.652, 139.485, 484.348);

    return fract(seed * sin(p)) * 2.0 - vec3(1.0);
}


float noise3(vec3 p)
{
    float f = (sqrt(4.0) - 1.0) / 3.0;
    mat3 skew = mat3(1.0 + f, f, f,
                     f, 1.0 + f, f,
					 f, f, 1.0 + f);
    float g = (1.0 - 1.0/sqrt(4.0)) / 3.0;
    mat3 invskew = mat3(1.0-g, -g, -g,
                        -g, 1.0-g, -g,
						-g, -g, 1.0-g);

    vec3 sp = skew * p;
    vec3 cell = floor(sp);
    vec3 d0 = fract(sp);

    float x0 = step(d0.x, d0.y);
	float x1 = step(d0.y, d0.z);
	float x2 = step(d0.z, d0.x);
	// x = x2*(1-x0)
    // y = x0*(1-x1)
    // z = x1*(1-x2)
    vec3 s0 = vec3(x2*(1.-x0), x0*(1.-x1), x1*(1.-x2));
    // x = min(1.0, x2+1.0-x0)
    // y = min(1.0, x0+1.0-x1)
    // z = min(1.0, x1+1.0-x2)
    vec3 s1 = min(vec3(1.0), vec3(1.0) + vec3(x2-x0, x0-x1, x1-x2));

    vec3 sv[4] = vec3[4](cell,
                         cell + s0,
                         cell + s1,
                         cell + vec3(1.0));
    vec3 wv[4] = vec3[4](invskew * sv[0],
                         invskew * sv[1],
                         invskew * sv[2],
                         invskew * sv[3]);
    vec3 d[4] = vec3[4](p - wv[0],
                        p - wv[1],
                        p - wv[2],
                        p - wv[3]);

    vec4 weights = max(vec4(0.0), vec4(0.6) - vec4(dot(d[0], d[0]),
                                                   dot(d[1], d[1]),
                                                   dot(d[2], d[2]),
                                                   dot(d[3], d[3])));
    weights = weights * weights * weights * weights;

    return (dot(hash3(sv[0]), d[0]) * weights[0] +
            dot(hash3(sv[1]), d[1]) * weights[1] +
            dot(hash3(sv[2]), d[2]) * weights[2] +
            dot(hash3(sv[3]), d[3]) * weights[3]) * 16.0;
}

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

void imageMain(inout vec4 frag_color, vec2 frag_coord)
{
    vec2 uv = frag_coord / iResolution.xy;
    uv.x *= iResolution.x / iResolution.y;

    vec3 d2 = hash3(vec3(1.0, -13.0, 0.0)) * iTime * .5;

	float aspect_ratio = iResolution.y / iResolution.x;
	float fov = 60.0 * 3.1415926536 / 180.0;
	float near = 0.1;
	vec3 half_diagonal = target_half_diagonal_hfov(near, fov, aspect_ratio);
	vec2 clip_coord = ((frag_coord / iResolution) - 0.5) * 2.0;

    vec3 ro = vec3(0.0, 0.0, -2.0);
	vec3 rd = compute_ray_plane(half_diagonal, clip_coord);

    vec3 pc = ro + dot(rd, -ro) * rd;
    float x0 = length(pc);
    float x1 = 1 - x0*x0;
    float x2 = length(pc - ro) - x1;

    vec3 i0 = ro + rd*x2;
    vec3 i1 = i0 + 2.0*(pc - i0);

    float t0 = x2;
    float t1 = length(ro - i1);
    float stepcount = 3.0;
    float step = (t1-t0)/stepcount;
    if (x0 < 1.0)
        for (float i = 0.0; i <= stepcount; i += 1.0)
    {
        float t = t0 + step*i;
        vec3 p = ro + t*rd;

        vec3 color = vec3(0.0);
        for (float i = 1.0; i < pow(2.0, 5.0); i *= 2.0)
            color.x += noise3((p*5.0 + d2/(i*1.2))*i) / i;

        color = color*4.0 + vec3(0.1);

        frag_color.xyz += color.xxx / stepcount;
    }

    frag_color = vec4(frag_color.x * 1.2, frag_color.x * 0.92, sqrt(2.0) * 0.3, 1.0);
    frag_color *= exp(-pow(length((frag_coord / iResolution.xy) - vec2(0.5)), 2.0) / 0.25);
}
