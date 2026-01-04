#include "m_pd.h"
#include <math.h>
#include <stdio.h>
#include "simplex_noise.h"
#define PI 3.14159265358979
#define max(x,y) ( x>y?x:y )
#define min(x,y) ( x<y?x:y )
#define clamp(x, low, high) (((x) > (high)) ? (high) : (((x) < (low)) ? (low) : (x)))

static t_class* glottis_tilde;

typedef struct _glottis_tilde {

	t_object x_obj;
	
	t_float lambda;
	t_float noiseSource;

	t_int sampleRate;
	t_int autoWobble;
	t_int alwaysVoice;

	t_float timeInWaveform;
	t_float totalTime;
	t_float intensity;
	t_float UITenseness;
	t_float UIFrequency;
	t_float loudness;
	//need to obeserved
	t_float oldFrequency;
	t_float newFrequency;
	t_float smoothFrequency;
	t_float oldTenseness;
	t_float newTenseness;
	//not used directly but after assigned a value in the program
	t_float alpha;
	t_float E0;
	t_float epsilon;
	t_float shift;
	t_float Delta;
	t_float Te;
	t_float omega;

	t_float waveformLength;   //new added,not existed in original code

	t_outlet* out;

} t_glottis_tilde;

void setupWaveform(t_floatarg lambda, t_glottis_tilde* x)
{
	//change:this.frequency --> new one
	t_float frequency = x->oldFrequency * (1 - lambda) + x->newFrequency * lambda;
	t_float tenseness = x->oldTenseness * (1 - lambda) + x->newTenseness * lambda;
	//here has change
	t_float Rd = 3 * (1 - tenseness);
	x->waveformLength = 1.0 / frequency;

	//t_float Rd = gt->Rd;
	if (Rd < 0.5) Rd = 0.5;
	if (Rd > 2.7) Rd = 2.7;
	t_float output;
	// normalized to time = 1, Ee = 1
	t_float Ra = -0.01 + 0.048 * Rd;
	t_float Rk = 0.224 + 0.118 * Rd;
	t_float Rg = (Rk / 4) * (0.5 + 1.2 * Rk) / (0.11 * Rd - Ra * (0.5 + 1.2 * Rk));

	t_float Ta = Ra;
	t_float Tp = 1 / (2 * Rg);
	t_float Te = Tp + Tp * Rk; //

	t_float epsilon = 1 / Ta;
	t_float shift = exp(- epsilon * (1 - Te));
	t_float Delta = 1 - shift; //divide by this to scale RHS

	t_float RHSIntegral = (1 / epsilon) * (shift - 1) + (1 - Te) * shift;
	RHSIntegral = RHSIntegral / Delta;

	t_float totalLowerIntegral = -(Te - Tp) / 2 + RHSIntegral;
	t_float totalUpperIntegral = -totalLowerIntegral;

	t_float omega = PI / Tp;
	t_float s = sin(omega * Te);
	// need E0*e^(alpha*Te)*s = -1 (to meet the return at -1)
	// and E0*e^(alpha*Tp/2) * Tp*2/pi = totalUpperIntegral
	//             (our approximation of the integral up to Tp)
	// writing x for e^alpha,
	// have E0*x^Te*s = -1 and E0 * x^(Tp/2) * Tp*2/pi = totalUpperIntegral
	// dividing the second by the first,
	// letting y = x^(Tp/2 - Te),
	// y * Tp*2 / (pi*s) = -totalUpperIntegral;
	t_float y = -PI * s * totalUpperIntegral / (Tp * 2);
	t_float z = log(y);
	t_float alpha = z / (Tp / 2 - Te);
	t_float E0 = -1 / (s * exp(alpha * Te));
	x->alpha = alpha;
	x->E0 = E0;
	x->epsilon = epsilon;
	x->shift = shift;
	x->Delta = Delta;
	x->Te = Te;
	x->omega = omega;

}

t_float normalizedLFWaveform(t_floatarg t, t_glottis_tilde * x)
{
	t_float output;
	if (t > x->Te) {
		output = (-exp(-x->epsilon * (t - x->Te)) + x->shift) / x->Delta;
	}
	else {
		output = x->E0 * exp(x->alpha * t) * sin(x->omega * t);
	}

	return output * x->intensity * x->loudness;
}

t_float getNoiseModulator(t_glottis_tilde * x)
{
	t_float voiced = 0.1 + 0.2 * max(0, sin(PI * 2 * x->timeInWaveform / x->waveformLength));
	//return 0.3;
	return x->UITenseness * x->intensity * voiced + (1 - x->UITenseness * x->intensity) * 0.3;
}

t_float runStep(t_floatarg lambda, t_floatarg noiseSource, t_glottis_tilde* x)
{
	t_float timeStep = 1.0 / x->sampleRate;
	x->timeInWaveform += timeStep;
	x->totalTime += timeStep;
	if (x->timeInWaveform > x->waveformLength)
	{
		x->timeInWaveform -= x->waveformLength;
		setupWaveform(lambda, x);
	}
	t_float out = normalizedLFWaveform(x->timeInWaveform / x->waveformLength, x);
	t_float aspiration = x->intensity * (1 - sqrt(x->UITenseness)) * getNoiseModulator(x) * noiseSource;
	aspiration *= 0.2 + 0.02 * simplex1(x->totalTime * 1.99);
	out += aspiration;
	return out;
}

void finishblock(t_glottis_tilde * x)
{
	t_float vibrato = 0;
	//vibratoAmount could be input
	t_float vibratoAmount = 0.005;
	t_float vibratoFrequency = 6;
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

void* glottis_tilde_new(t_symbol* s, int argc, t_atom* argv)
{
	t_glottis_tilde* x = (t_glottis_tilde*)pd_new(glottis_tilde);
	x->sampleRate = 48000;
	x->autoWobble = 1;
	x->alwaysVoice = 1;

	x->timeInWaveform = 0;
	x->totalTime = 0;
	x->waveformLength = 0;
	x->UIFrequency = 140;
	x->oldFrequency = 140;
	x->newFrequency = 140;
	x->smoothFrequency = 140;
	x->UITenseness = 0.6;
	x->oldTenseness = 0.6;
	x->newTenseness = 0.6;
	x->intensity = 0;
	x->loudness = 1;

	
	inlet_new(&x->x_obj, &x->x_obj.ob_pd,
		gensym("global"), gensym("UIFrequency"), gensym("UITenseness"), gensym("finish"));
	floatinlet_new(&x->x_obj, &x->lambda);
	floatinlet_new(&x->x_obj, &x->noiseSource);
	x->out = outlet_new(&x->x_obj, &s_float);
	return (void*)x;
}

void glottis_global(t_glottis_tilde* x, t_floatarg f1, t_floatarg f2, t_floatarg f3) {
	x->sampleRate = f1;
	x->autoWobble = f2;
	x->alwaysVoice = f3;
}
void glottis_frequency(t_glottis_tilde* x, t_floatarg f) {
	x->UIFrequency = f;
	if (x->intensity == 0) {
		x->smoothFrequency = x->UIFrequency;
	}
}
void glottis_tenseness(t_glottis_tilde* x, t_floatarg f) {
	x->UITenseness = f;
	x->loudness = pow(x->UITenseness, 0.25);
}
void glottis_bang(t_glottis_tilde* x)
{
	t_float outf = runStep(x->lambda, x->noiseSource, x);

	outlet_float(x->out, outf);
}

void glottis_setup(void) {
	glottis_tilde = class_new(gensym("glottis"),
		(t_newmethod)glottis_tilde_new,
		0, sizeof(t_glottis_tilde),
		CLASS_DEFAULT,
		A_DEFFLOAT, 0);
	// Each class that has a method for the dsp-message is recognized as signalclass.
	/*class_addmethod(glottis_tilde,
		(t_method)glottis_tilde_dsp, gensym("dsp"), 0);*/
	//Signalclasses that want to provide signal-inlets have to declare this via the	CLASS_MAINSIGNALIN - macro.
	//The first argument of the macro is a pointer to the signalclass.
	//The second argument is the type of the class's dataspace
	//The last argument is a dummy-variable out of the dataspace that is needed to replace 
	//non - existing signal at the signal - inlet(s) with t_floatmessages.
	//CLASS_MAINSIGNALIN(glottis_tilde, t_glottis_tilde, lambda);
	class_addbang(glottis_tilde, glottis_bang);
	class_addmethod(glottis_tilde,
		(t_method)glottis_global, gensym("global"), A_DEFFLOAT, A_DEFFLOAT, A_DEFFLOAT, 0);
	class_addmethod(glottis_tilde,
		(t_method)glottis_frequency, gensym("UIFrequency"), A_DEFFLOAT, 0);
	class_addmethod(glottis_tilde,
		(t_method)glottis_tenseness, gensym("UITenseness"), A_DEFFLOAT, 0);
	class_addmethod(glottis_tilde,
		(t_method)finishblock, gensym("finish"), 0);
}
