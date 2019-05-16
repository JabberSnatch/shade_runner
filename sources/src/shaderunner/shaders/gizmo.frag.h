/*
 * ----------------------------------------------------------------------------
 * "THE BEER-WARE LICENSE" (Revision 42):
 * Samuel Bourasseau wrote this file. You can do whatever you want with this
 * stuff. If we meet some day, and you think this stuff is worth it, you can
 * buy me a beer in return.
 * ----------------------------------------------------------------------------
 */

R"__SR_SS__(

layout(location = 0) out vec4 frag_color;
layout(location = 1) out float id_map;

uniform vec3 uGizmoColor;
uniform int uGizmoID;

void main()
{
    frag_color = vec4(uGizmoColor, 1.0);
    id_map = intBitsToFloat(uGizmoID);
}

)__SR_SS__"
