#include"m_pd.h"
#include<math.h>
#define PI 3.14159265358979
#define clamp(x, low, high) (((x) > (high)) ? (high) : (((x) < (low)) ? (low) : (x)))
static t_class* visualizeXY_class;
typedef struct _visualizeXY
{
	t_object x_obj;
	t_float xx;
	t_float yy;
	t_outlet* out, * a_out, *b_out;
}t_visual;

void visual_bound(t_visual *x, t_floatarg f1, t_floatarg f2)
{
	outlet_bang(x->a_out);
	x->xx = f1;
	x->yy = f2;
	double xx = clamp(x->xx, 55, 160);
	double yy = clamp(x->yy, 60, 165);
	double tempRadius = sqrt(xx * xx + yy * yy);
	if (tempRadius > 175)
	{
		tempRadius = 175;
		yy = sqrt(tempRadius * tempRadius - xx * xx);
	}
	xx = 0 - xx;
	yy = 0 - yy;

	double radius = 298;
	double scale = 60;

	double tongueIndex = 12.9;
	double tongueDiameter = 2.43;
	double innerTongueControlRadius = 2.05;
	double outerTongueControlRadius = 3.5;
	double tongueLowerIndexBound = 10 + 2;
	double tongueUpperIndexBound = 39 - 3;
	double tongueIndexCentre = 0.5 * (tongueLowerIndexBound + tongueUpperIndexBound);
	double angleOffset = -0.24;
	//calculate diameter
	tongueDiameter = (radius - tempRadius) / scale;


	//calculate index
	double angle = atan2(yy, xx);
	while (angle > 0) angle -= 2 * PI;
	tongueIndex = (PI + angle - angleOffset) * (39.0 - 1) / (0.64 * PI); //angleScale = 0.64

	//normalize tongueIndex
	double fromPoint = (outerTongueControlRadius - tongueDiameter) / (outerTongueControlRadius - innerTongueControlRadius);
	fromPoint = clamp(fromPoint, 0, 1);
	fromPoint = pow(fromPoint, 0.58) - 0.2 * (fromPoint * fromPoint - fromPoint); //horrible kludge to fit curve to straight line
	tongueDiameter = clamp(tongueDiameter, innerTongueControlRadius, outerTongueControlRadius);
	double out = fromPoint * 0.5 * (tongueUpperIndexBound - tongueLowerIndexBound);
	tongueIndex = clamp(tongueIndex, tongueIndexCentre - out, tongueIndexCentre + out);
	t_float diameter_i;
	for (int i = 0; i < 10; i++) {
		if (i<7)
		{
			diameter_i = 0.6;
			outlet_float(x->out, diameter_i);
		}
		else
		{
			diameter_i = 1.1;
			outlet_float(x->out, diameter_i);
		}
		
	}
	for (int i = 10; i < 39; i++)
	{
		double m = 0;
		double t = 1.1 * PI * (tongueIndex - i) / (32.0 - 10);
		double fixedTongueDiameter = 2 + (tongueDiameter - 2) / 1.5;
		double curve = (1.5 - fixedTongueDiameter + 1.7) * cos(t);  //gridoffset = 1.7
		if (i == 10 - 2 || i == 39 - 1) curve *= 0.8;
		if (i == 10 || i == 39 - 2) curve *= 0.94;
		diameter_i = 1.5 - curve;
		outlet_float(x->out, diameter_i);
	}
	for (int i = 39; i < 44; i++)
	{
		diameter_i = 1.5;
		outlet_float(x->out, diameter_i);
	}
	outlet_bang(x->b_out);
}


void* visual_new(t_symbol* s, int argc, t_atom* argv) {
	t_visual* x = (t_visual*)pd_new(visualizeXY_class);
	x->xx = 55;
	x->yy = 60;
	inlet_new(&x->x_obj, &x->x_obj.ob_pd,
		gensym("list"), gensym("bound"));

	x->out = outlet_new(&x->x_obj, &s_float);
	x->a_out = outlet_new(&x->x_obj, &s_bang);
	x->b_out = outlet_new(&x->x_obj, &s_bang);
	return (void*)x;
}


void visualizeXY_setup(void) {
	visualizeXY_class = class_new(gensym("visualizeXY"),
		(t_newmethod)visual_new,
		0, sizeof(t_visual),
		CLASS_DEFAULT,
		A_DEFFLOAT, 0);
	class_addmethod(visualizeXY_class,
		(t_method)visual_bound, gensym("bound"),A_DEFFLOAT, A_DEFFLOAT, 0);

}
