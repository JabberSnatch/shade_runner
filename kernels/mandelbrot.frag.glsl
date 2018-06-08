vec3 HSLtoRGB(vec3 hsl)
{
	vec3 hue = clamp(vec3(
		abs(hsl.x * 6.0 - 3.0) - 1.0,
		2.0 - abs(hsl.x * 6.0 - 2.0),
		2.0 - abs(hsl.x * 6.0 - 4.0)), 0.0, 1.0);
	float C = (1.0 - abs(2.0 * hsl.z - 1.0)) * hsl.y;
	return (hue - vec3(0.5)) * C + hsl.zzz;
}

vec2 complex_product(vec2 lhs, vec2 rhs)
{
    return vec2(lhs.x * rhs.x - lhs.y * rhs.y,
                lhs.y * rhs.x + lhs.x * rhs.y);
}
 
vec4 compute_bounds(vec2 horizontal_bounds, float vertical_center, float aspect_ratio)
{
    float vertical_range = (horizontal_bounds.y - horizontal_bounds.x) / aspect_ratio;
    float vertical_half_range = vertical_range / 2.0;
    return vec4(horizontal_bounds, vec2(vertical_center - vertical_half_range, vertical_center + vertical_half_range));
}

vec2 pos_in_bounds(vec2 uv, vec4 bounds)
{
    vec2 range = vec2(bounds.y - bounds.x, bounds.w - bounds.z);
    return bounds.xz + range * uv;
}

vec2 mandelbrot_step(vec2 zn, vec2 c)
{
    return complex_product(zn, zn) + c;
}

vec2 mandelbrot_dstep(vec2 zn, vec2 dzn)
{
    return complex_product(vec2(2.0, 0.0), complex_product(zn, dzn)) + vec2(1.0, 0.0);
}

float sin_tween(vec2 bounds, float t)
{
    float range = bounds.y - bounds.x;
    float normalized_sin_t = sin(t) * 0.5 + 0.5;
    return bounds.x + range * normalized_sin_t;
}

void imageMain( out vec4 fragColor, in vec2 fragCoord )
{
    vec2 uv = fragCoord / iResolution;
    float aspect_ratio = iResolution.x / iResolution.y;
    vec2 view_center = vec2(-0.5, 0.0);
    float horizontal_extent = 3.0;
    
    float set_scale = 1.0;
    
    if (true)
    {
    	set_scale = 2.0;
    	view_center = vec2(-1.52, 2.479);
    	horizontal_extent = 0.00002;
    	horizontal_extent = exp(sin_tween(vec2(log(horizontal_extent), log(12.0)), iTime));
    }
    else if (false)
    {
    	view_center = vec2(-0.37998, 0.62);
    	horizontal_extent = 0.00001;
    	horizontal_extent = exp(sin_tween(vec2(log(horizontal_extent), log(3.0)), iTime));
    }
        
    //view_center = vec2(-0.815, (sin(iTime) + 1.0) / 2.0 * 0.06);
    //horizontal_extent = 0.01;
    //view_center = vec2(-1.35003, 0.05002);
    //horizontal_extent = (sin(iTime) + 1.1) * 0.1 / 2.0;
    //horizontal_extent = 0.00001;
    
    float horizontal_half_extent = horizontal_extent / 2.0;
    vec2 horizontal_bounds = vec2(view_center.x - horizontal_half_extent, view_center.x + horizontal_half_extent);
    vec4 bounds = compute_bounds(horizontal_bounds, view_center.y, aspect_ratio);
    vec2 position = pos_in_bounds(uv, bounds);
    
    vec2 zn = vec2(0);
    //zn = vec2(cos(iTime / 5.0), sin(iTime / 2.5));
    
    if (false)
    {
    bool rejected = false;
    int iteration_loop = 250;
    int iteration_count = 500;
    for (int i = 0; i < iteration_count && !rejected; ++i)
    {
        zn = mandelbrot_step(zn, position) / set_scale;
        rejected = length(zn) > 2.0 * set_scale;
        if (rejected)
        {
            float hue = fract((float(i) / float(iteration_loop)) + sin(iTime) + 0.6);
            fragColor = vec4(HSLtoRGB(vec3(hue, 1.0, 0.5)), 1.0);
        }
    }
    }
    else
    {
        vec2 dzn = vec2(0);
        int iteration_count = 1000;
        bool rejected = false;
        for (int i = 0; i < iteration_count && !rejected; ++i)
        {
            dzn = mandelbrot_dstep(zn, dzn) / set_scale;
            zn = mandelbrot_step(zn, position) / set_scale;
            rejected = length(zn) > pow(64.0, set_scale);
        }
        float distance = 0.5 * (length(zn) / length(dzn)) * log(length(zn));
        distance = sign(distance) * pow(10.0 * distance / horizontal_extent, 0.2);
		vec3 color = vec3(0.0);
        if (rejected)
        {
            color = HSLtoRGB(vec3(distance, 1.0, 0.5));
            //color = vec3(distance);
        }
        fragColor = vec4(color, 1.0);
    }
}
