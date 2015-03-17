//
//  scanslide~.c
//  
//
//  Created by Cheng-i Wang on 10/14/13.
//  Copyright (c) 2013 __MyCompanyName__. All rights reserved.
//

#include <m_pd.h>

static t_class *scanslide_class;

typedef struct _scanslide{
	t_object x_obj;
	t_float x_slide;
	t_float output;
}t_scanslide;

static t_int *scanslide_perform(t_int *w){
	t_scanslide *x = (t_scanslide*)(w[1]);
	t_float *in = (t_float *)(w[2]);
	t_float *out = (t_float *)(w[3]);
	int n = (int)(w[4]);
	while (n--) {
		float f = *(in++);
		x->x_slide = (x->x_slide < 1.0f) ? 1.0f:x->x_slide;
		*out = (x->output) + (f - (x->output))/(x->x_slide);
		x->output = *out;
		out++;
	}
	
	return(w+5);
}

static void scanslide_dsp(t_scanslide *x, t_signal **sp){
	dsp_add(scanslide_perform, 4, x, sp[0]->s_vec, sp[1]->s_vec, sp[0]->s_n);
}

static void *scanslide_new(void){
	t_scanslide *x = (t_scanslide *)pd_new(scanslide_class);
	outlet_new(&x->x_obj, gensym("signal"));
	x->x_slide = 1.0f;
	x->output = 0.0f;
	
	return(x);
}

void scanslide_tilde_setup(void){
	scanslide_class = class_new(gensym("scanslide~"), (t_newmethod)scanslide_new, 0, sizeof(t_scanslide), 0, A_GIMME, 0);
	CLASS_MAINSIGNALIN(scanslide_class, t_scanslide, x_slide);
	class_addmethod(scanslide_class, (t_method)scanslide_dsp, gensym("dsp"), 0);
}