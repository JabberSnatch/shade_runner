void imageMain(inout vec4 frag_color, vec2 frag_coord)
{
	mat2 skew = mat2(1, 0.0,
                     0.5, sqrt(0.75));
    mat2 invskew = inverse(skew);
    vec2 uv = frag_coord / iResolution.xy;
    uv.x *= iResolution.x / iResolution.y;
    uv *= 25.0;

    uv -= skew * vec2(iTime * -0.0, -iTime * 0.5) * 5.0;

    vec2 coord = fract(uv*2.0)*2.0;
    coord = invskew * uv;
    coord *= 1.0;
    vec2 x0 = fract(coord);
    coord = coord - fract(coord);
    float x1 = trunc(x0.x + x0.y);
    coord = abs(coord) + x1;

    mat2 colormat = mat2(.3427, .25664,
                         .1883, .283);
    vec2 x2 = fract(coord/2.0);

    vec2 color = vec2(1.0-step(x2.x+x2.y, 0.0));

    float x3 = step(2.5, mod(iTime, 3.14)) * step(0.15, mod(iTime, 0.4)) * step(0.02, mod(iTime, 0.18));
    color = mix(color, fract(colormat*coord), x3);

	frag_color = vec4(color * 0.7, sqrt(2.0) * 0.3, 1.0);

    frag_color *= exp(-pow(length((frag_coord / iResolution.xy) - vec2(0.5)), 2.0) / 0.25);
}
