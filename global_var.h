#include <stdio.h>
#include"t_Stack.h"
#define PI 3.14159265358979
#define max(x,y) ( x>y?x:y )
#define min(x,y) ( x<y?x:y )
#define clamp(x, low, high) (((x) > (high)) ? (high) : (((x) < (low)) ? (low) : (x)))
#define moveTowards4(current, target, amountUp, amountDown) ((current < target)? (min(current + amountUp, target)):(max(current - amountDown, target)))
#define moveTowards3(current, target, amount) ((current < target) ? (min(current + amount, target)): (max(current - amount, target)))
typedef struct global_varibles {

	//double blockTime = 1;
	//int blockLength = 512;
	////bool:started,soundOn
	//int started = 0;
	//int soundOn = 0;
	double blockTime;
	int blockLength;
	//bool:started,soundOn
	int started;
	int soundOn;

	//Glottis varibles
	double lambda;
	double noiseSource;

	int sampleRate;
	int autoWobble;
	int alwaysVoice;

	double timeInWaveform;
	double totalTime;
	double intensity;
	double UITenseness;
	double UIFrequency;
	double loudness;
	//need to be obeserved
	double oldFrequency;
	double newFrequency;
	double smoothFrequency;
	double oldTenseness;
	double newTenseness;
	//not used directly but after assigned a value in the program
	double alpha;
	double E0;
	double epsilon;
	double shift;
	double Delta;
	double Te;
	double omega;

	double waveformLength;

	//Tract variables
	int n;
	double fade;
	int lastObstruction;
	double velumTarget;
	double tipStart;

	Sqstack* transients;
	double* R;
	double* L;
	double* junctionOutputR;
	double* junctionOutputL;
	double* reflection;
	double* newReflection;
	double glottalReflection;
	double lipReflection;
	double lipOutput;

	double* maxAmplitude;
	double* diameter;
	double* restDiameter;
	double* targetDiameter;
	double* newDiameter;

	double* A;

	double movementSpeed;
	int noseLength;
	int noseStart;
	double noseOutput;
	double* noseR;
	double* noseL;
	double* noseJunctionOutputR;
	double* noseJunctionOutputL;
	double* noseReflection;
	double* noseDiameter;
	double* noseA;
	double* noseMaxAmplitude;

	double newReflectionLeft;
	double newReflectionRight;
	double newReflectionNose;
	double reflectionLeft;
	double reflectionRight;
	double reflectionNose;
}t_global;

void init_global(t_global* x)
{
	//Glottis
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

	//tract
	x->transients = t_InitStack();
	x->n = 44;
	x->glottalReflection = 0.75;
	x->lipReflection = -0.85;
	x->lipOutput = 0;
	x->fade = 1.0;
	x->noseOutput = 0;
	x->movementSpeed = 15;
	x->lastObstruction = -1;
	x->velumTarget = 0.01;
	x->tipStart = 32;

	x->tipStart = floor(x->tipStart * x->n / 44);

	x->diameter = (double*)malloc(x->n * sizeof(double));
	x->restDiameter = (double*)malloc(x->n * sizeof(double));
	x->targetDiameter = (double*)malloc(x->n * sizeof(double));;
	x->newDiameter = (double*)malloc(x->n * sizeof(double));

	x->A = (double*)malloc(x->n * sizeof(double));

	x->R = (double*)malloc(x->n * sizeof(double));
	x->L = (double*)malloc(x->n * sizeof(double));
	x->junctionOutputR = (double*)malloc(sizeof(double) * (x->n + 1));
	x->junctionOutputL = (double*)malloc(sizeof(double) * (x->n + 1));
	x->reflection = (double*)malloc(sizeof(double) * (x->n + 1));
	x->newReflection = (double*)malloc(sizeof(double) * (x->n + 1));
	x->maxAmplitude = (double*)malloc(x->n * sizeof(double));

	x->noseLength = floor(28 * x->n / 44);
	x->noseStart = x->n - x->noseLength + 1;
	x->noseR = (double*)malloc(x->noseLength * sizeof(double));
	x->noseL = (double*)malloc(x->noseLength * sizeof(double));
	x->noseJunctionOutputR = (double*)malloc((x->noseLength + 1) * sizeof(double));
	x->noseJunctionOutputL = (double*)malloc((x->noseLength + 1) * sizeof(double));
	x->noseReflection = (double*)malloc((x->noseLength + 1) * sizeof(double));
	x->noseDiameter = (double*)malloc(x->noseLength * sizeof(double));
	x->noseA = (double*)malloc(x->noseLength * sizeof(double));
	x->noseMaxAmplitude = (double*)malloc(x->noseLength * sizeof(double));

	x->newReflectionLeft = x->newReflectionRight = x->newReflectionNose = 0;
}
extern double Glottis_runStep(double lambda, double noiseSource, t_global* x);
extern void Glottis_finishblock(t_global* x);
extern void tract_runStep(double glottalOutput, double turbulenceNoise, double lambda, t_global* x);
extern void tract_finishBlock(t_global* x);
