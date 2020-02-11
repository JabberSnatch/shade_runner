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
layout(triangle_strip, max_vertices = 72) out;

out vec3 gsVertColor;

uniform mat4 uProjectionMat;

void main()
{
    vec4 in_position = gl_in[0].gl_Position;

#define EMIT_LOCAL(position) gl_Position = uProjectionMat * (in_position + (position) * extent); gsVertColor = color; EmitVertex()
#define EMIT_CUBE(position)\
	EMIT_LOCAL(position + vec4(-0.5, -0.5, -0.5, 0));\
	EMIT_LOCAL(position + vec4(0.5, -0.5, -0.5, 0));\
	EMIT_LOCAL(position + vec4(-0.5, -0.5, 0.5, 0));\
	EMIT_LOCAL(position + vec4(0.5, -0.5, 0.5, 0));\
	EMIT_LOCAL(position + vec4(0.5, 0.5, 0.5, 0));\
	EMIT_LOCAL(position + vec4(0.5, -0.5, -0.5, 0));\
	EMIT_LOCAL(position + vec4(0.5, 0.5, -0.5, 0));\
	EMIT_LOCAL(position + vec4(-0.5, -0.5, -0.5, 0));\
	EMIT_LOCAL(position + vec4(-0.5, 0.5, -0.5, 0));\
	EMIT_LOCAL(position + vec4(-0.5, -0.5, 0.5, 0));\
	EMIT_LOCAL(position + vec4(-0.5, 0.5, 0.5, 0));\
	EMIT_LOCAL(position + vec4(0.5, 0.5, 0.5, 0));\
	EMIT_LOCAL(position + vec4(-0.5, 0.5, -0.5, 0));\
	EMIT_LOCAL(position + vec4(0.5, 0.5, -0.5, 0));\
	EndPrimitive()

    vec4 extent = vec4(1.0, 0.05, 0.05, 0.0);
    vec3 color = vec3(1.0, 0.0, 0.0);
    EMIT_CUBE(vec4(0.475, 0.0, 0.0, 0.0));

    extent = vec4(0.05, 1.0, 0.05, 0.0);
    color = vec3(0.0, 1.0, 0.0);
    EMIT_CUBE(vec4(0.0, 0.475, 0.0, 0.0));

    extent = vec4(0.05, 0.05, 1.0, 0.0);
    color = vec3(0.0, 0.0, 1.0);
    EMIT_CUBE(vec4(0.0, 0.0, 0.475, 0.0));
}

)__SR_SS__"
