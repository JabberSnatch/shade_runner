/*
 * ----------------------------------------------------------------------------
 * "THE BEER-WARE LICENSE" (Revision 42):
 * Samuel Bourasseau wrote this file. As long as you retain this notice you
 * can do whatever you want with this stuff. If we meet some day, and you think
 * this stuff is worth it, you can buy me a beer in return.
 * ----------------------------------------------------------------------------
 */

R"__SR_SS__(

out vec4 frag_color;

void main()
{
	frag_color = vec4(0.0);
	vec2 frag_coord = floor(gl_FragCoord).xy;
	SR_FRAG_ENTRY_POINT(frag_color, frag_coord);
}

)__SR_SS__"
