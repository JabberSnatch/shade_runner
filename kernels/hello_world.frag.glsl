/*
 * ----------------------------------------------------------------------------
 * "THE BEER-WARE LICENSE" (Revision 42):
 * Samuel Bourasseau wrote this file. As long as you retain this notice you
 * can do whatever you want with this stuff. If we meet some day, and you think
 * this stuff is worth it, you can buy me a beer in return.
 * ----------------------------------------------------------------------------
 */


void imageMain(inout vec4 frag_color, vec2 frag_coord) {
	vec2 uv = frag_coord / vec2(1280.0, 720.0);
	frag_color.xz = abs(vec2(1.0) - abs(uv * 2.0 - vec2(1.0)));
	frag_color.a = 1.0;
}
