vec2 VertexColor(vec2 cell)
{
    mat2 colormat = mat2(.3427, .25664,
                         .1883, .283);
    return fract(colormat*cell);
}

float VertexIntensity(vec2 cell, vec2 disp)
{
    mat2 colormat = mat2(.879 + (cos(iTime*3.) + 1.0) * 0.1, .2563 + (sin(iTime*3.) + 1.0) * 0.1,
                         .1883, .7783 + (sin(iTime) + 1.0) * 0.1);
    return dot(disp, fract(colormat*cell) * 2.0 + vec2(1.0)) * 0.5 + 1.0;
}

vec2 VertexGradient(vec2 cell)
{
    //return vec2(1.0, 1.0);// * (mod(cell.y, 2.0) * 2.0 - 1.0);
    //return hash3(cell).xy;
#if 1
    /*
    mat2 colormat = mat2(.879 + (cos(iTime*3.) + 1.0) * 0.1, .2563 + (sin(iTime*3.) + 1.0) * 0.1,
                         .1883, .7783 + (sin(iTime) + 1.0) * 0.1);
    */
    mat2 colormat = mat2(742.342, 823.457,
                         247.999, 530.343);

    return fract(colormat * sin(cell)) * 2.0 - vec2(1.0);
#endif

    mat2 seed = mat2(.4, .5,
                     .3, .4) * 10.0;
    float modu = 7919.0 * 6983.0;
    vec2 v = cell * vec2(1.0, 0.0) + cell * vec2(1.0, 1.0);
    return mod(seed * cell, modu) / modu;
}

float simplexnoise(vec2 uv)
{
    float f = (sqrt(3.0) - 1.0) / 2.0;
    mat2 skew = mat2(1.0 + f, f,
                     f, 1.0 + f);
    float g = (1.0 - 1.0/sqrt(3.0)) / 2.0;
    mat2 invskew = mat2(1.0-g, -g,
                        -g, 1.0-g);

    vec2 coord = skew * uv;
    vec2 cell = floor(coord);
    vec2 x0 = fract(coord);
    float x1 = step(x0.x, x0.y);

    vec2 neighbours[3] = vec2[3](
        cell,
        cell + vec2(1.0 - x1, x1),
        cell + vec2(1.0, 1.0));
    vec2 x2[3] = vec2[3](
        invskew * neighbours[0],
        invskew * neighbours[1],
        invskew * neighbours[2]);
    vec2 disp[3] = vec2[3](
        uv - x2[0],
        uv - x2[1],
        uv - x2[2]);
    vec3 distances = vec3(
        length(disp[0]),
        length(disp[1]),
        length(disp[2]));

    vec3 weights = max(vec3(0.0), vec3(0.6) - vec3(
        dot(disp[0], disp[0]),
        dot(disp[1], disp[1]),
        dot(disp[2], disp[2])));
    weights = weights * weights * weights * weights;

    return (dot(VertexGradient(neighbours[0]), disp[0]) * weights[0] +
            dot(VertexGradient(neighbours[1]), disp[1]) * weights[1] +
            dot(VertexGradient(neighbours[2]), disp[2]) * weights[2]) * 70.0;
}

void imageMain(inout vec4 frag_color, vec2 frag_coord)
{
    vec2 uv = frag_coord / iResolution.xy;
    uv.x *= iResolution.x / iResolution.y;
    uv *= 0.5 * vec2(5.0, 5.0);
    //uv += vec2(-4.0);

    float f = (sqrt(3.0) - 1.0) / 2.0;
    mat2 skew = mat2(1.0 + f, f,
                     f, 1.0 + f);
    vec2 coord = skew * uv;
    vec2 cell = floor(coord);
    vec2 x0 = fract(coord);
    float x1 = step(x0.x, x0.y);

    vec2 d0 = VertexGradient(vec2(234.097, 443.895)) * sin(iTime/5.0) * 10.0;
    vec2 d1 = VertexGradient(vec2(-2.0, 801.3)) * sin(iTime/3.0) * 10.0;
    vec2 d2 = VertexGradient(vec2(1.0, -13.0)) * sin(iTime/7.0) * 10.0;

    vec3 color = vec3(0.0);
    //color.x -= simplexnoise(uv + d2);
    for (float i = 1.0; i < pow(2.0, 10.0); i *= 2.0)
    {
        color.x += simplexnoise((uv + d2)*i) / i;
        //color.y += simplexnoise((uv + d0)*i) / i;
        //color.z += simplexnoise((uv + d1)*i) / i;
    }
    color = color / 2.0;
    color = max(color - vec3(0.5), 0.0) * 1.0;
    //color = color * 0.5 + vec3(0.5);

    frag_color = vec4(color, 1.0);
//frag_color = vec4(color * 0.7, sqrt(2.0) * 0.3, 1.0);

//frag_color *= exp(-pow(length((frag_coord / iResolution.xy) - vec2(0.5)), 2.0) / 0.25);
}
