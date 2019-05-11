/*
 * ----------------------------------------------------------------------------
 * "THE BEER-WARE LICENSE" (Revision 42):
 * Samuel Bourasseau wrote this file. You can do whatever you want with this
 * stuff. If we meet some day, and you think this stuff is worth it, you can
 * buy me a beer in return.
 * ----------------------------------------------------------------------------
 */

R"__SR_SS__(

layout(points) in;
layout(triangle_strip, max_vertices = 24) out;

uniform mat4 uProjectionMat;

const vec4 kBaseExtent = 0.1 * vec4(1.0, 1.0, 1.0, 0.0);

void main()
{
    vec4 in_position = gl_in[0].gl_Position;

#define EMIT_LOCAL(position) gl_Position = uProjectionMat * (in_position + (position) * kBaseExtent); EmitVertex()
	EMIT_LOCAL(vec4(-1, -1, -1, 0));
	EMIT_LOCAL(vec4(1, -1, -1, 0));
	EMIT_LOCAL(vec4(-1, -1, 1, 0));
	EMIT_LOCAL(vec4(1, -1, 1, 0));

	EMIT_LOCAL(vec4(1, 1, 1, 0));

	EMIT_LOCAL(vec4(1, -1, -1, 0));
	EMIT_LOCAL(vec4(1, 1, -1, 0));

	EMIT_LOCAL(vec4(-1, -1, -1, 0));
	EMIT_LOCAL(vec4(-1, 1, -1, 0));

	EMIT_LOCAL(vec4(-1, -1, 1, 0));
	EMIT_LOCAL(vec4(-1, 1, 1, 0));

	EMIT_LOCAL(vec4(1, 1, 1, 0));

	EMIT_LOCAL(vec4(-1, 1, -1, 0));
	EMIT_LOCAL(vec4(1, 1, -1, 0));
	EndPrimitive();
}

)__SR_SS__"
