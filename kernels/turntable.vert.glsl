in vec3 in_position;

void vertexMain(inout vec4 vert_position)
{
	vert_position = vec4(in_position * (2.0 * (cos(iTime) + 1.0)), 1.0);
}