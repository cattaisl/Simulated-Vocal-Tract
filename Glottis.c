#include <math.h>
#include <stdio.h>
#include "simplex_noise.h"
#include "global_var.h"

//typedef struct _glottis_tilde {
//
//	double lambda;
//	double noiseSource;
//
//	int sampleRate;
//	int autoWobble;
//	int alwaysVoice;
//
//	double timeInWaveform;
//	double totalTime;
//	double intensity;
//	double UITenseness;
//	double UIFrequency;
//	double loudness;
//	//need to obeserved
//	double oldFrequency;
//	double newFrequency;
//	double smoothFrequency;
//	double oldTenseness;
//	double newTenseness;
//	//not used directly but after assigned a value in the program
//	double alpha;
//	double E0;
//	double epsilon;
//	double shift;
//	double Delta;
//	double Te;
//	double omega;
//
//	double waveformLength;   //new added,not existed in original code
//
//} t_global;

void setupWaveform(double lambda, t_global* x)
{
	//change:this.frequency --> new one
	double frequency = x->oldFrequency * (1 - lambda) + x->newFrequency * lambda;
	double tenseness = x->oldTenseness * (1 - lambda) + x->newTenseness * lambda;
	//here has change
	double Rd = 3 * (1 - tenseness);
	x->waveformLength = 1.0 / frequency;

	//double Rd = gt->Rd;
	if (Rd < 0.5) Rd = 0.5;
	if (Rd > 2.7) Rd = 2.7;
	double output;
	// normalized to time = 1, Ee = 1
	double Ra = -0.01 + 0.048 * Rd;
	double Rk = 0.224 + 0.118 * Rd;
	double Rg = (Rk / 4) * (0.5 + 1.2 * Rk) / (0.11 * Rd - Ra * (0.5 + 1.2 * Rk));

	double Ta = Ra;
	double Tp = 1 / (2 * Rg);
	double Te = Tp + Tp * Rk; //

	double epsilon = 1 / Ta;
	double shift = exp(-epsilon * (1 - Te));
	double Delta = 1 - shift; //divide by this to scale RHS

	double RHSIntegral = (1 / epsilon) * (shift - 1) + (1 - Te) * shift;
	RHSIntegral = RHSIntegral / Delta;

	double totalLowerIntegral = -(Te - Tp) / 2 + RHSIntegral;
	double totalUpperIntegral = -totalLowerIntegral;

	double omega = PI / Tp;
	double s = sin(omega * Te);
	// need E0*e^(alpha*Te)*s = -1 (to meet the return at -1)
	// and E0*e^(alpha*Tp/2) * Tp*2/pi = totalUpperIntegral
	//             (our approximation of the integral up to Tp)
	// writing x for e^alpha,
	// have E0*x^Te*s = -1 and E0 * x^(Tp/2) * Tp*2/pi = totalUpperIntegral
	// dividing the second by the first,
	// letting y = x^(Tp/2 - Te),
	// y * Tp*2 / (pi*s) = -totalUpperIntegral;
	double y = -PI * s * totalUpperIntegral / (Tp * 2);
	double z = log(y);
	double alpha = z / (Tp / 2 - Te);
	double E0 = -1 / (s * exp(alpha * Te));
	x->alpha = alpha;
	x->E0 = E0;
	x->epsilon = epsilon;
	x->shift = shift;
	x->Delta = Delta;
	x->Te = Te;
	x->omega = omega;

}

double normalizedLFWaveform(double t, t_global * x)
{
	double output;
	if (t > x->Te) {
		output = (-exp(-x->epsilon * (t - x->Te)) + x->shift) / x->Delta;
	}
	else {
		output = x->E0 * exp(x->alpha * t) * sin(x->omega * t);
	}

	return output * x->intensity* x->loudness;
}

double getNoiseModulator(t_global * x)
{
	double voiced = 0.1 + 0.2 * max(0, sin(PI * 2 * x->timeInWaveform / x->waveformLength));
	//return 0.3;
	return x->UITenseness * x->intensity * voiced + (1 - x->UITenseness * x->intensity) * 0.3;
}

double Glottis_runStep(double lambda, double noiseSource, t_global * x)
{
	double timeStep = 1.0 / x->sampleRate;
	x->timeInWaveform += timeStep;
	x->totalTime += timeStep;
	if (x->timeInWaveform > x->waveformLength)
	{
		x->timeInWaveform -= x->waveformLength;
		setupWaveform(lambda, x);
	}
	double out = normalizedLFWaveform(x->timeInWaveform / x->waveformLength, x);
	double aspiration = x->intensity * (1 - sqrt(x->UITenseness)) * getNoiseModulator(x) * noiseSource;
	aspiration *= 0.2 + 0.02 * simplex1(x->totalTime * 1.99);
	out += aspiration;
	return out;
}

void Glottis_finishblock(t_global * x)
{
	double vibrato = 0;
	//vibratoAmount could be input
	double vibratoAmount = 0.005;
	double vibratoFrequency = 6;
	vibrato += vibratoAmount * sin(2 * PI * x->totalTime * vibratoFrequency);
	vibrato += 0.02 * simplex1(x->totalTime * 4.07);
	vibrato += 0.04 * simplex1(x->totalTime * 2.15);
	if (x->autoWobble)
	{
		vibrato += 0.2 * simplex1(x->totalTime * 0.98);
		vibrato += 0.4 * simplex1(x->totalTime * 0.5);
	}
	if (x->UIFrequency > x->smoothFrequency)
		x->smoothFrequency = min(x->smoothFrequency * 1.1, x->UIFrequency);
	if (x->UIFrequency < x->smoothFrequency)
		x->smoothFrequency = max(x->smoothFrequency / 1.1, x->UIFrequency);
	x->oldFrequency = x->newFrequency;
	x->newFrequency = x->smoothFrequency * (1 + vibrato);
	x->oldTenseness = x->newTenseness;
	x->newTenseness = x->UITenseness
		+ 0.1 * simplex1(x->totalTime * 0.46) + 0.05 * simplex1(x->totalTime * 0.36);
	//if (!this.isTouched && x->alwaysVoice) x->newTenseness += (3 - x->UITenseness) * (1 - x->intensity);
	if (x->alwaysVoice) {
		x->newTenseness += (3 - x->UITenseness) * (1 - x->intensity);
		x->intensity += 0.13;
	}
	//if (this.isTouched || x->alwaysVoice) x->intensity += 0.13;
	else {
		x->intensity -= 0.05;
	}
	x->intensity = clamp(x->intensity, 0, 1);
}

//void* glottis_new(t_global* x)
//{
//	
//}

void glottis_global(t_global * x, double f1, double f2, double f3) {
	x->sampleRate = f1;
	x->autoWobble = f2;
	x->alwaysVoice = f3;
}
void glottis_frequency(t_global * x, double f) {
	x->UIFrequency = f;
	if (x->intensity == 0) {
		x->smoothFrequency = x->UIFrequency;
	}
}
void glottis_tenseness(t_global * x, double f) {
	x->UITenseness = f;
	x->loudness = pow(x->UITenseness, 0.25);
}

