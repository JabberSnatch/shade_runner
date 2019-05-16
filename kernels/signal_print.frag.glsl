// ================================================================================
// MATHS
// ================================================================================

#define PI 3.1415926535


// ================================================================================
// MAIN
// ================================================================================

const float kBpm = 80.0;
#define PULSEp(bpm, signature, release) (fract(iTime * bpm / 60.0 / signature) < release)
#define PULSE(time, bpm, signature) fract(time * bpm / 60.0 / signature)
#define SAWTOOTH(pulse, freq, signature) (fract(pulse * freq / 60.0 * signature))
#define OHtTZ(v) ((v) * 2.0 - 1.0)
#define TZtOH(v) ((v) * 0.5 + 0.5)

float sawtooth(float time, float bpm, float signature)
{
	return fract((time * bpm) / (60.0 * signature));
}

#if 0
float pulse(float time, float signature)
{
	return SAWTOOTH(time, kBpm, signature);
	return TZtOH((sin(OHtTZ(SAWTOOTH(time, kBpm, 1.0)) * PI)));
}
#endif

float dirac(float x, float time, float bpm, float signature)
{
	return 1.0 - step(step(sawtooth(time, bpm, signature), 0.01), x);
}


float plot(float x, float y, float r)
{
	float top = step(y - r, x);
	float bot = 1.0 - step(y + r, x);
	return top * bot;
}

float pulse(float time, float bpm, float signature)
{
	return fract((time * bpm) / (60.0 * signature));
}

float cycle(float time, float bpm, float signature, float release)
{
	float pulse = pulse(time, bpm, signature);
	return step(pulse, release) * (pulse / release);
}

float envelope(float t, float attack, float decay, float sustain)
{
	float attack_segment = (t / attack) * step(t, attack);
	float decay_segment = mix(1.0, sustain, (t-attack)/decay) * step(t-attack, decay) * step(attack, t);
	float sustain_segment = sustain * step(decay, t-attack);
	return attack_segment + decay_segment + sustain_segment;
}


void imageMain(inout vec4 frag_color, vec2 frag_coord)
{
	vec2 uv = frag_coord.xy / iResolution.xy;

	float tn = 0.005;
	float htn = tn * 0.5;
	float tx = iTime * 0.25 + uv.x;
	//tx = uv.x + 0.7 * 60.0 / kBpm;

	float rep = iTime / kBpm;
	float v = cycle(tx, kBpm, 0.5, 0.9);
	frag_color.x = plot(uv.y, envelope(v, 0.1, 0.1, 0.5), htn);
	frag_color.z = plot(uv.y, envelope(cycle(tx, kBpm, 3.0, 0.33333 * 0.5), 0.1, 0.3, 0.7), htn);
	frag_color.y = dirac(uv.y, tx, kBpm, 1.0);
	//frag_color.z = plot(uv.y, pulse(tx, .25), htn);
}