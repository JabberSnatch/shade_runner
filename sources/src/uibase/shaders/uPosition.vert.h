/*
 * ----------------------------------------------------------------------------
 * "THE BEER-WARE LICENSE" (Revision 42):
 * Samuel Bourasseau wrote this file. You can do whatever you want with this
 * stuff. If we meet some day, and you think this stuff is worth it, you can
 * buy me a beer in return.
 * ----------------------------------------------------------------------------
 */

R"__SR_SS__(

uniform vec3 uPosition;

void main()
{
    gl_Position = vec4(uPosition, 1.0);
}

)__SR_SS__"

