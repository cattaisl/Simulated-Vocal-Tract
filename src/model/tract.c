#include <math.h>
#include <stdio.h>
#include<stdlib.h>
#include "global_var.h"
double gaussian() {
	int s = 0;
	//srand((unsigned)time(NULL));
	for (int c = 0; c < 16; c++) s += rand();
	return (s - 8) / 4;
}




//typedef struct _tract {
//
//	int sampleRate;
//	int n;
//	double fade;
//	int lastObstruction;
//	double velumTarget;
//
//	Sqstack *transients;
//	double* R;
//	double* L;
//	double* junctionOutputR;
//	double* junctionOutputL;
//	double* reflection;
//	double* newReflection;
//	double glottalReflection;
//	double lipReflection;
//	double lipOutput;
//
//	double* maxAmplitude;
//	double* diameter;
//	double* restDiameter;
//	double* targetDiameter;
//	double* newDiameter;
//
//	double* A;
//
//	double movementSpeed;
//	int noseLength;
//	int noseStart;
//	double noseOutput;
//	double* noseR;
//	double *noseL;
//	double* noseJunctionOutputR;
//	double* noseJunctionOutputL;
//	double* noseReflection;
//	double* noseDiameter;
//	double* noseA;
//	double* noseMaxAmplitude;
//
//	double newReflectionLeft;
//	double newReflectionRight;
//	double newReflectionNose;
//	double reflectionLeft;
//	double reflectionRight;
//	double reflectionNose;
//} t_global;

//void tract_init(t_global *x)
//{
//	x->transients = t_InitStack();
//	x->n = 44;
//	x->sampleRate = 48000;
//	x->glottalReflection = 0.75;
//	x->lipReflection = -0.85;
//	x->lipOutput = 0;
//	x->fade = 1.0;
//	x->noseOutput = 0;
//	x->movementSpeed = 15;
//	x->lastObstruction = -1;
//	x->velumTarget = 0.01;
//
//	tipStart = floor(tipStart * x->n / 44);
//
//	x->diameter = (double*)malloc(x->n * sizeof(double));
//	x->restDiameter = (double*)malloc(x->n * sizeof(double));
//	x->targetDiameter = (double*)malloc(x->n * sizeof(double));;
//	x->newDiameter = (double*)malloc(x->n * sizeof(double));
//
//	x->A = (double*)malloc(x->n * sizeof(double));
//
//	x->R = (double*)malloc(x->n * sizeof(double));
//	x->L = (double*)malloc(x->n * sizeof(double));
//	x->junctionOutputR = (double*)malloc(sizeof(double) * (x->n + 1));
//	x->junctionOutputL = (double*)malloc(sizeof(double) * (x->n + 1));
//	x->reflection = (double*)malloc(sizeof(double) * (x->n + 1));
//	x->newReflection = (double*)malloc(sizeof(double) * (x->n + 1));
//	x->maxAmplitude = (double*)malloc(x->n * sizeof(double));
//
//	x->noseLength = floor(28 * x->n / 44);
//	x->noseStart = x->n - x->noseLength + 1;
//	x->noseR = (double*)malloc(x->noseLength * sizeof(double));
//	x->noseL = (double*)malloc(x->noseLength * sizeof(double));
//	x->noseJunctionOutputR = (double*)malloc((x->noseLength + 1) * sizeof(double));
//	x->noseJunctionOutputL = (double*)malloc((x->noseLength + 1) * sizeof(double));
//	x->noseReflection = (double*)malloc((x->noseLength + 1) * sizeof(double));
//	x->noseDiameter = (double*)malloc(x->noseLength * sizeof(double));
//	x->noseA = (double*)malloc(x->noseLength * sizeof(double));
//	x->noseMaxAmplitude = (double*)malloc(x->noseLength * sizeof(double));
//
//	x->newReflectionLeft = x->newReflectionRight = x->newReflectionNose = 0;
//}

void addTransient(int position, t_global* x)
{
	t_trans trans;
	trans.position = position;
	trans.timeAlive = 0;
	trans.lifeTime = 0.2;
	trans.strength = 0.3;
	trans.exponent = 200;
	t_Push(x->transients, trans);
}

void processTransients(t_global *x)
{
	for (int i = 0; i < x->transients->top; i++)
	{
		t_trans trans = x->transients->a[i];
		double amplitude = trans.strength * pow(2, -trans.exponent * trans.timeAlive);
		x->R[trans.position] += amplitude / 2;
		x->L[trans.position] += amplitude / 2;
		trans.timeAlive += 1.0 / (x->sampleRate * 2);
	}
	for (int i = x->transients->top - 1; i >= 0; i--)
	{
		t_trans trans = x->transients->a[i];
		if (trans.timeAlive > trans.lifeTime)
		{
			t_splice(x->transients,i, 1);
		}
	}
}

void reshapeTract(t_global* x)
{
	double deltaTime = x->blockTime;
	double amount = deltaTime * x->movementSpeed; ;
	double newLastObstruction = -1;
	for (int i = 0; i < x->n; i++)
	{
		double diameter = x->diameter[i];
		double targetDiameter = x->targetDiameter[i];
		if (diameter <= 0) newLastObstruction = i;
		double slowReturn;
		if (i < x->noseStart) slowReturn = 0.6;
		else if (i >= x->tipStart) slowReturn = 1.0;
		else slowReturn = 0.6 + 0.4 * (i - x->noseStart) / (x->tipStart - x->noseStart);
		x->diameter[i] = moveTowards4(diameter, targetDiameter, slowReturn * amount, 2 * amount);
	}
	if (x->lastObstruction > -1 && newLastObstruction == -1 && x->noseA[0] < 0.05)
	{
		addTransient(x->lastObstruction,x);
	}
	x->lastObstruction = newLastObstruction;

	amount = deltaTime * x->movementSpeed;
	x->noseDiameter[0] = moveTowards4(x->noseDiameter[0], x->velumTarget,
		amount * 0.25, amount * 0.1);
	x->noseA[0] = x->noseDiameter[0] * x->noseDiameter[0];
}

void calculateReflections(t_global* x)
{
	for (int i = 0; i < x->n; i++)
	{
		x->A[i] = x->diameter[i] * x->diameter[i]; //ignoring PI etc.
	}
	for (int i = 1; i < x->n; i++)
	{
		x->reflection[i] = x->newReflection[i];
		if (x->A[i] == 0) x->newReflection[i] = 0.999; //to prevent some bad behaviour if 0
		else x->newReflection[i] = (x->A[i - 1] - x->A[i]) / (x->A[i - 1] + x->A[i]);
	}

	//now at junction with nose

	x->reflectionLeft = x->newReflectionLeft;
	x->reflectionRight = x->newReflectionRight;
	x->reflectionNose = x->newReflectionNose;
	double sum = x->A[x->noseStart] + x->A[x->noseStart + 1] + x->noseA[0];
	x->newReflectionLeft = (2 * x->A[x->noseStart] - sum) / sum;
	x->newReflectionRight = (2 * x->A[x->noseStart + 1] - sum) / sum;
	x->newReflectionNose = (2 * x->noseA[0] - sum) / sum;
}

void calculateNoseReflections(t_global* x)
{
	for (int i = 0; i < x->noseLength; i++)
	{
		x->noseA[i] = x->noseDiameter[i] * x->noseDiameter[i];
	}
	for (int i = 1; i < x->noseLength; i++)
	{
		x->noseReflection[i] = (x->noseA[i - 1] - x->noseA[i]) / (x->noseA[i - 1] + x->noseA[i]);
	}
}

void addTurbulenceNoise(double turbulenceNoise, t_global* x)
{
	//for (int j = 0; j < touchesWithMouse.length; j++)
	//{
		/*var touch = UI.touchesWithMouse[j];
		if (touch.index < 2 || touch.index > x->n) continue;
		if (touch.diameter <= 0) continue;
		var intensity = touch.fricative_intensity;
		if (intensity == 0) continue;
		addTurbulenceNoiseAtIndex(0.66 * turbulenceNoise * intensity, touch.index, touch.diameter,x);*/

	//}
}

void addTurbulenceNoiseAtIndex(double turbulenceNoise, double index, double diameter, t_global* x)
{
	int i = floor(index);
	double delta = index - i;
	turbulenceNoise *= getNoiseModulator(x);
	double thinness0 = clamp(8 * (0.7 - diameter), 0, 1);
	double openness = clamp(30 * (diameter - 0.3), 0, 1);
	double noise0 = turbulenceNoise * (1 - delta) * thinness0 * openness;
	double noise1 = turbulenceNoise * delta * thinness0 * openness;
	x->R[i + 1] += noise0 / 2;
	x->L[i + 1] += noise0 / 2;
	x->R[i + 2] += noise1 / 2;
	x->L[i + 2] += noise1 / 2;
}

void tract_runStep(double glottalOutput, double turbulenceNoise, double lambda, t_global* x)
{
	randomize();
	double updateAmplitudes = (random(100) * 0.01);

	//mouth
	processTransients(x);
	addTurbulenceNoise(turbulenceNoise,x);

	x->junctionOutputR[0] = x->L[0] * x->glottalReflection + glottalOutput;
	x->junctionOutputL[x->n] = x->R[x->n - 1] * x->lipReflection;

	for (int i = 1; i < x->n; i++)
	{
		double r = x->reflection[i] * (1 - lambda) + x->newReflection[i] * lambda;
		double w = r * (x->R[i - 1] + x->L[i]);
		x->junctionOutputR[i] = x->R[i - 1] - w;
		x->junctionOutputL[i] = x->L[i] + w;
	}

	//now at junction with nose
	int i = x->noseStart;
	double r = x->newReflectionLeft * (1 - lambda) + x->reflectionLeft * lambda;
	x->junctionOutputL[i] = r * x->R[i - 1] + (1 + r) * (x->noseL[0] + x->L[i]);
	r = x->newReflectionRight * (1 - lambda) + x->reflectionRight * lambda;
	x->junctionOutputR[i] = r * x->L[i] + (1 + r) * (x->R[i - 1] + x->noseL[0]);
	r = x->newReflectionNose * (1 - lambda) + x->reflectionNose * lambda;
	x->noseJunctionOutputR[0] = r * x->noseL[0] + (1 + r) * (x->L[i] + x->R[i - 1]);

	for (int i = 0; i < x->n; i++)
	{
		x->R[i] = x->junctionOutputR[i] * 0.999;
		x->L[i] = x->junctionOutputL[i + 1] * 0.999;

		if (updateAmplitudes)
		{
			double amplitude = fabs(x->R[i] + x->L[i]);
			if (amplitude > x->maxAmplitude[i]) x->maxAmplitude[i] = amplitude;
			else x->maxAmplitude[i] *= 0.999;
		}
	}

	x->lipOutput = x->R[x->n - 1];

	//nose
	x->noseJunctionOutputL[x->noseLength] = x->noseR[x->noseLength - 1] * x->lipReflection;

	for (int i = 1; i < x->noseLength; i++)
	{
		double w = x->noseReflection[i] * (x->noseR[i - 1] + x->noseL[i]);
		x->noseJunctionOutputR[i] = x->noseR[i - 1] - w;
		x->noseJunctionOutputL[i] = x->noseL[i] + w;
	}

	for (int i = 0; i < x->noseLength; i++)
	{
		x->noseR[i] = x->noseJunctionOutputR[i] * x->fade;
		x->noseL[i] = x->noseJunctionOutputL[i + 1] * x->fade;


		if (updateAmplitudes)
		{
			double amplitude = fabs(x->noseR[i] + x->noseL[i]);
			if (amplitude > x->noseMaxAmplitude[i]) x->noseMaxAmplitude[i] = amplitude;
			else x->noseMaxAmplitude[i] *= 0.999;
		}
	}

	x->noseOutput = x->noseR[x->noseLength - 1];
}

void tract_finishBlock(t_global* x)
{
	reshapeTract(x);
	calculateReflections(x);
}
