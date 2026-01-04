#include <stdio.h>
#include"m_pd.h"
#include"global_var.h"


static t_class* audio_class;
typedef struct _audioSystem 
{
	t_object x_obj;
	t_sample f;
	
}t_audio;

void* audio_new(t_symbol* s, int argc, t_atom* argv) {
	t_audio* x = (t_audio*)pd_new(audio_class);
	inlet_new(&x->x_obj, &x->x_obj.ob_pd, &s_signal, &s_signal);
	outlet_new(&x->x_obj, &s_signal);
	return (void*)x;
}

//void audio_bang(t_audio* x)
//{
//	//outlet_float(x->out, outf);
//}
//t_int* pan_tilde_perform(t_int* w)
//{
//	t_audio* x = (t_audio*)(w[1]);
//	t_sample* in1 = (t_sample*)(w[2]);
//	t_sample* in2 = (t_sample*)(w[3]);
//	t_sample* out = (t_sample*)(w[4]);
//	int n = (int)(w[5]);
//	t_sample f_pan = (x->f_pan < 0) ? 0.0 : (x->f_pan > 1) ? 1.0 : x->f_pan;
//	while (n--)* out++ = (*in1++) * (1 - f_pan) + (*in2++) * f_pan;
//	return (w + 6);
//}
t_int* doScriptProcessor(t_int* w)
{
	t_audio* x = (t_audio*)(w[1]);
	t_sample* inputArray1 = (t_sample*)(w[2]);
	t_sample* inputArray2 = (t_sample*)(w[3]);
	t_sample* outArray = (t_sample*)(w[4]);
	int n = (int)(w[5]);
	t_global* g = (t_global*)malloc(sizeof(t_global));
	init_global(g);
	for (int j = 0, N = sizeof(outArray) / sizeof(outArray[0]); j < N; j++)
	{
		double lambda1 = j / N;
		double lambda2 = (j + 0.5) / N;
		double glottalOutput = Glottis_runStep(lambda1, inputArray1[j],g);

		double vocalOutput = 0;
		//Tract runs at twice the sample rate
		tract_runStep(glottalOutput, inputArray2[j], lambda1,g);
		vocalOutput += g->lipOutput + g->noseOutput;
		tract_runStep(glottalOutput, inputArray2[j], lambda2,g);
		vocalOutput += g->lipOutput + g->noseOutput;
		outArray[j] = vocalOutput * 0.125;
	}
	Glottis_finishblock(g);
	tract_finishBlock(g);

	return (w + 6);
}
void audio_tilde_dsp(t_audio* x, t_signal** sp)
{
	//Here, sp[0] is the first in-signal, sp[1] represents the second in-signal and sp[3] points to the out - signal.
	dsp_add(doScriptProcessor, 5, x,
		sp[0]->s_vec, sp[1]->s_vec, sp[2]->s_vec, sp[0]->s_n);
}
void audio_setup(void) {
	audio_class = class_new(gensym("glottis"),
		(t_newmethod)audio_new,
		0, sizeof(t_audio),
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
	//class_addbang(audio_class, audio_bang);
	class_addmethod(audio_class,
		(t_method)audio_tilde_dsp, gensym("dsp"), 0);
	//class_addmethod(audio_class,
	//	(t_method)glottis_global, gensym("global"), A_DEFFLOAT, A_DEFFLOAT, A_DEFFLOAT, 0);
	CLASS_MAINSIGNALIN(audio_class, t_audio, f);

}
