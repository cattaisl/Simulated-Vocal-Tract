#include"m_pd.h"
#include<math.h>
#define PI 3.14159265358979
#define clamp(x, low, high) (((x) > (high)) ? (high) : (((x) < (low)) ? (low) : (x)))
static t_class * FormantTransfer_class;
typedef struct _FormantTransfer
{
	t_object x_obj;
	t_outlet* out, * a_out, * b_out;
}t_formant;
void changeByFormant(t_formant* x, t_floatarg f1, t_floatarg f2)
{
	outlet_bang(x->a_out);
	double omegai[] = { 0.636,0.561,0.561,0.550,0.598,0.895,1.187,1.417,1.380,1.273,1.340,
		1.399,1.433,1.506,1.493,1.473,1.499,1.529,1.567,1.601,1.591,1.547,1.570,1.546,
		1.532,1.496,1.429,1.425,1.496,1.608,1.668,1.757,1.842,1.983,2.073,2.123,2.194,
		2.175,2.009,1.785,1.675,1.539,1.405,1.312 };
	double phi1[] = { 0.018,0.001,-0.013,-0.025,-0.036,-0.048,-0.062,-0.076,-0.093,-0.111,
		-0.130,-0.149,-0.167,-0.183,-0.196,-0.204,-0.207,-0.203,-0.193,-0.175,-0.151,-0.119,
		-0.082,-0.041,0.004,0.051,0.097,0.141,0.181,0.214,0.240,0.257,0.264,0.260,0.246,0.224,
		0.194,0.159,0.122,0.087, 0.057, 0.038, 0.034, 0.048};
	double phi2[] = { -0.013,-0.007,-0.029,-0.059,-0.088,-0.108,-0.120,-0.123,-0.118,-0.107,
		-0.092,-0.075,-0.056,-0.035,-0.014,0.008,0.032,0.057,0.084,0.111,0.138,0.164,0.188,
		0.206,0.218,0.221,0.214,0.195,0.164,0.121,0.070,0.013,-0.046,-0.100,-0.143,-0.167,
		-0.165,-0.132,-0.066,-0.031,0.148,0.264,0.346,0.338};

	double p1 = f1;
	double p2 = f2;
	double diameter_i;

	for (int i = 1; i < 44; i++)
	{

		diameter_i = omegai[i] + p1 * phi1[i] + p2 * phi2[i];
		//in case some bad behaviours
		if (diameter_i < 0.3)
		{
			diameter_i = 0.3;
		}
		outlet_float(x->out, diameter_i);
	}
	outlet_bang(x->b_out);
}



void* formant_new(t_symbol * s, int argc, t_atom * argv) {
	t_formant* x = (t_formant*)pd_new(FormantTransfer_class);
	inlet_new(&x->x_obj, &x->x_obj.ob_pd,
		gensym("list"), gensym("bound"));
	x->out = outlet_new(&x->x_obj, &s_float);
	x->a_out = outlet_new(&x->x_obj, &s_bang);
	x->b_out = outlet_new(&x->x_obj, &s_bang);
	return (void*)x;
}


void FormantTransfer_setup(void) {
	FormantTransfer_class = class_new(gensym("FormantTransfer"),
		(t_newmethod)formant_new,
		0, sizeof(t_formant),
		CLASS_DEFAULT,
		A_DEFFLOAT, 0);
	class_addmethod(FormantTransfer_class,
		(t_method)changeByFormant, gensym("bound"), A_DEFFLOAT, A_DEFFLOAT, 0);

}
