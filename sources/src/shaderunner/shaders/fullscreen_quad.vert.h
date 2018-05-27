/*
 * ----------------------------------------------------------------------------
 * "THE BEER-WARE LICENSE" (Revision 42):
 * Samuel Bourasseau wrote this file. As long as you retain this notice you
 * can do whatever you want with this stuff. If we meet some day, and you think
 * this stuff is worth it, you can buy me a beer in return.
 * ----------------------------------------------------------------------------
 */

R"__SR_SS__(

const vec2 kQuadVertices[] = vec2[6](
	vec2(-1.0, -1.0), vec2(1.0, -1.0), vec2(1.0, 1.0),
	vec2(1.0, 1.0), vec2(-1.0, 1.0), vec2(-1.0, -1.0)
);

void main()
{
	gl_Position = vec4(kQuadVertices[gl_VertexID], 0.0, 1.0);
}

)__SR_SS__"
