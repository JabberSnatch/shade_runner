vec2 VertexColor(vec2 cell)
{
    mat2 colormat = mat2(.3427, .25664,
                         .1883, .283);
    return fract(colormat*cell);
}

float VertexIntensity(vec2 cell, vec2 disp)
{
    mat2 colormat = mat2(.879, .00002563,
                         .00018083, .7783);
    return dot(disp, normalize(fract(colormat*cell) * 2.0 + vec2(1.0))) * 0.5 + 1.0;
}

void imageMain(inout vec4 frag_color, vec2 frag_coord)
{
	mat2 skew = mat2(1, 0.0,
                     0.5, sqrt(0.75));

#if 1
    float f = (sqrt(3.0) - 1.0) / 2.0;
    skew = mat2(1.0 + f, f,
                f, 1.0 + f);
#endif
    float g = (1.0 - 1.0/sqrt(3.0)) / 2.0;
    mat2 invskew = mat2(1.0-g, -g,
                        -g, 1.0-g);

    //skew = inverse(skew);
#if 0
    mat2 tmp = invskew;
    invskew = skew;
    skew = tmp;
#endif
    vec2 uv = frag_coord / iResolution.xy;
    uv.x *= iResolution.x / iResolution.y;
    uv *= 25.0;

    //uv -= skew * vec2(iTime * 0.5, -iTime * 0.5) * 5.0;

    vec2 coord = fract(uv*2.0)*2.0;
    coord = skew * uv;

    vec2 cell = floor(coord);
    vec2 x0 = fract(coord);
    float x1 = step(x0.x, x0.y);

    vec2 neighbours[3] = vec2[3](
        cell,
        cell + vec2(1.0 - x1, x1),
        cell + vec2(1.0, 1.0));
    vec2 bary = mix(x0, 1.0 - x0, x1);

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

    vec3 weights = max(vec3(0.0), vec3(0.5*0.5) - distances*distances);
    weights = weights * weights * weights * weights;
    //weights = distances;
    weights = normalize(weights);

    vec2 color = vec2(
        VertexIntensity(neighbours[0], disp[0]) * weights.x +
        VertexIntensity(neighbours[1], disp[1]) * weights.y +
        VertexIntensity(neighbours[2], disp[2]) * weights.z);
    //color = vec2(min(distances.x, min(distances.y, distances.z))) * 2.0;
    //color = bary;
    float w0 = step(weights.x, weights.y);
    float w1 = step(weights.y, weights.z);
    float w2 = step(weights.x, weights.z);
    //color = VertexColor(neighbours[0]) * w0*w2 + VertexColor(neighbours[1]) * w1*(1.0-w0) + VertexColor(neighbours[2]) * (1.0-w1)*(1.0-w2);
    //color = weights.xz;
#if 0
    mat2 colormat = mat2(.3427, .25664,
                         .1883, .283);
    coord = cell*2.0 + vec2(x1);
    vec2 x2 = fract(coord/2.0);

    vec2 color = vec2(1.0-step(x2.x+x2.y, 0.0));
    color = fract(coord/10.0);

    float x3 = step(2.5, mod(iTime, 3.14)) * step(0.15, mod(iTime, 0.4)) * step(0.02, mod(iTime, 0.18));
    //color = mix(color, fract(colormat*coord), x3);

#endif
	frag_color = vec4(color * 0.7, sqrt(2.0) * 0.3, 1.0);

    frag_color *= exp(-pow(length((frag_coord / iResolution.xy) - vec2(0.5)), 2.0) / 0.25);
}
