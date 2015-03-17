//
//  scanslide.c
//  
//
//  Created by Cheng-i Wang on 10/8/13.
//  Copyright (c) 2013 __MyCompanyName__. All rights reserved.
//

#include "m_pd.h"

typedef struct scanslide{
	t_object x_ob;
	t_outlet *x_outlet, *up_out, *down_out;
	t_float mode;
	t_float output; // variable for storing previous output value
	t_float slide_up;
	t_float slide_down;
}t_scanslide;

void scanslide_float(t_scanslide *x, t_floatarg f){
	/*Do delay here
	 y[n] = y[n-1] + (x[n]-y[n-1])/delayNum)
	 */
	t_float y;
	x->slide_up = (x->slide_up < 1.0f) ? 1.0f:x->slide_up;  	 
	x->slide_down = (x->slide_down < 1.0f) ? 1.0f:x->slide_down;  	 

	if (f >= x->output){
		y = (x->output) + (f - (x->output))/(x->slide_up); 		
	}else{
		y = (x->output) + (f - (x->output))/(x->slide_down); 				
	}
	x->output = y;
	outlet_float(x->x_outlet, x->output);
}

void scanslide_set(t_scanslide *x, t_symbol *s, int argc, t_atom *argv){
	switch (argc) {
		case 1:{
			t_float f;
			f = atom_getfloat(argv);
			switch ((int)x->mode) {
				case 0:
					x->slide_up = f;
					x->slide_down = f;
					outlet_float(x->up_out, f);
					outlet_float(x->down_out, f);			
					break;
				case 1:
					x->slide_up = f;
					outlet_float(x->up_out, f);
					break;
				case 2:
					x->slide_down = f;
					outlet_float(x->down_out, f);
					break;					
				default:
					x->slide_up = f;
					x->slide_down = f;
					outlet_float(x->up_out, f);
					outlet_float(x->down_out, f);			
					break;
			}
		}break;
		case 2:{
			t_float f1,f2; 
			f1 = atom_getfloat(argv);
			f2 = atom_getfloat(argv+1);
			x->slide_up = f1;
			x->slide_down = f2;
			outlet_float(x->up_out, f1);
			outlet_float(x->down_out, f2);			
		}break;
		default:
			break;
	}
}

void scanslide_mode(t_scanslide *x, t_float m){
	x->mode = m;
}

t_class *scanslide_class;

void *scanslide_new(t_symbol *s, int argc, t_atom *argv){
	t_scanslide *x = (t_scanslide *)pd_new(scanslide_class);
	x->output = 0.0f;
	x->mode = 0;
	switch (argc) {
		case 1:
			x->slide_up = atom_getfloat(argv);
			x->slide_down = atom_getfloat(argv);			
			break;
		case 2:
			x->slide_up = atom_getfloat(argv);
			x->slide_down = atom_getfloat(argv+1);
			break;
		default:
			break;
	}
	inlet_new(&x->x_ob, &x->x_ob.ob_pd, gensym("float"), gensym("set"));	
	x->x_outlet = outlet_new(&x->x_ob, gensym("float"));
	x->up_out = outlet_new(&x->x_ob, gensym("float"));
	x->down_out = outlet_new(&x->x_ob, gensym("float"));
	return (void *) x;
}

void scanslide_setup(void){
	scanslide_class = class_new(gensym("scanslide"), (t_newmethod)scanslide_new, 0, sizeof(t_scanslide),0, A_GIMME, 0);
	class_addmethod(scanslide_class, (t_method)scanslide_set, gensym("set"), A_GIMME, 0);
	class_addmethod(scanslide_class, (t_method)scanslide_mode, gensym("mode"), A_DEFFLOAT, 0);
	class_addfloat(scanslide_class, scanslide_float);
}