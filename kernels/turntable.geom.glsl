layout(points) in;
layout(triangle_strip, max_vertices = 24) out;

const vec4 kBaseExtent = 0.2 * vec4(1.0, 1.0, 1.0, 0.0);


const float pi = 3.1415926535;

mat4 perspective_hfov(float n, float f, float alpha, float aspect)
{
	float inverse_tan_half_alpha = 1.0 / tan(alpha * 0.5);
	return mat4(
		inverse_tan_half_alpha, 0.0, 0.0, 0.0,
		0.0, (1.0/aspect)*inverse_tan_half_alpha, 0.0, 0.0,
		0.0, 0.0, (-(f+n))/(f-n), -1.0,
		0.0, 0.0, (-2.0*f*n)/(f-n), 0.0
	);
}


void main()
{
	float aspect = iResolution.y / iResolution.x;
	float fov = 90.0 * pi / 180.0;
	float near = 0.01;
	float far = 1000.0;
	mat4 projection_matrix = perspective_hfov(near, far, fov, aspect);
	mat4 view_matrix = mat4(
		 cos(iTime), 0.0, sin(iTime), 0.0,
		 0.0, 1.0, 0.0, 0.0,
		 -sin(iTime), 0.0, cos(iTime), 0.0,
		 0.0, 0.0, 0.0, 1.0
	);
	mat4 translation_matrix = mat4(
		 1.0, 0.0, 0.0, 0.0,
		 0.0, 1.0, 0.0, 0.0,
		 0.0, 0.0, 1.0, 0.0,
		 0.0, 0.0, -1.5, 1.0
	);

	mat4 clip_space = projection_matrix * translation_matrix * view_matrix;

	vec4 in_position = gl_in[0].gl_Position;

	gl_Position = clip_space * (in_position + vec4(1, -1, 1, 0) * kBaseExtent); EmitVertex();
	gl_Position = clip_space * (in_position + vec4(-1, -1, 1, 0) * kBaseExtent); EmitVertex();
	gl_Position = clip_space * (in_position + vec4(1, -1, -1, 0) * kBaseExtent); EmitVertex();
	gl_Position = clip_space * (in_position + vec4(-1, -1, -1, 0) * kBaseExtent); EmitVertex();
	EndPrimitive();

	gl_Position = clip_space * (in_position + vec4(1, -1, -1, 0) * kBaseExtent); EmitVertex();
	gl_Position = clip_space * (in_position + vec4(1, 1, -1, 0) * kBaseExtent); EmitVertex();
	gl_Position = clip_space * (in_position + vec4(1, -1, 1, 0) * kBaseExtent); EmitVertex();
	gl_Position = clip_space * (in_position + vec4(1, 1, 1, 0) * kBaseExtent); EmitVertex();
	EndPrimitive();

	gl_Position = clip_space * (in_position + vec4(1, -1, 1, 0) * kBaseExtent); EmitVertex();
	gl_Position = clip_space * (in_position + vec4(1, 1, 1, 0) * kBaseExtent); EmitVertex();
	gl_Position = clip_space * (in_position + vec4(-1, -1, 1, 0) * kBaseExtent); EmitVertex();
	gl_Position = clip_space * (in_position + vec4(-1, 1, 1, 0) * kBaseExtent); EmitVertex();
	EndPrimitive();

	gl_Position = clip_space * (in_position + vec4(-1, -1, 1, 0) * kBaseExtent); EmitVertex();
	gl_Position = clip_space * (in_position + vec4(-1, 1, 1, 0) * kBaseExtent); EmitVertex();
	gl_Position = clip_space * (in_position + vec4(-1, -1, -1, 0) * kBaseExtent); EmitVertex();
	gl_Position = clip_space * (in_position + vec4(-1, 1, -1, 0) * kBaseExtent); EmitVertex();
	EndPrimitive();

	gl_Position = clip_space * (in_position + vec4(-1, -1, -1, 0) * kBaseExtent); EmitVertex();
	gl_Position = clip_space * (in_position + vec4(-1, 1, -1, 0) * kBaseExtent); EmitVertex();
	gl_Position = clip_space * (in_position + vec4(1, -1, -1, 0) * kBaseExtent); EmitVertex();
	gl_Position = clip_space * (in_position + vec4(1, 1, -1, 0) * kBaseExtent); EmitVertex();
	EndPrimitive();

	gl_Position = clip_space * (in_position + vec4(1, 1, -1, 0) * kBaseExtent); EmitVertex();
	gl_Position = clip_space * (in_position + vec4(-1, 1, -1, 0) * kBaseExtent); EmitVertex();
	gl_Position = clip_space * (in_position + vec4(1, 1, 1, 0) * kBaseExtent); EmitVertex();
	gl_Position = clip_space * (in_position + vec4(-1, 1, 1, 0) * kBaseExtent); EmitVertex();
	EndPrimitive();
}
