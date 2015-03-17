//
//  coin.c
//  
//
//  Created by Cheng-i Wang on 10/9/13.
//  Copyright (c) 2013 __MyCompanyName__. All rights reserved.
//

#include "m_pd.h"
#include "math.h"
#include "stdlib.h"
#include <time.h> 

typedef struct coin{
	t_object x_ob;
	t_outlet *x_outlet, *prob_out, *coin_out;
	t_float prob, t_value, f_value;
}t_coin;

void coin_bang(t_coin *x){
	t_float out;
	// Flip the coin here
	srand(time(NULL));
	t_float c = (t_float)(1 + rand()%99);
	if (c <= x->prob){
		out = x->t_value;
	}else{
		out = x->f_value;
	}
	outlet_float(x->x_outlet, out);
	outlet_float(x->coin_out, c);
}

void coin_set(t_coin *x, t_symbol *s, int argc, t_atom *argv){
	if (argc > 0) {
		t_float f = atom_getfloat(argv);
		if (f <= 0) {
			x->prob = 0.0f;
		}else if(f >= 100){
			x->prob = 100.0f;
		}else{
			x->prob = f;
		}
		switch (argc) {
			case 2:{
				x->t_value = atom_getfloat(argv+1);		
			}break;
			case 3:{
				x->t_value = atom_getfloat(argv+1);		
				x->f_value = atom_getfloat(argv+2);
			}break;
			default:
				break;
		}
		outlet_float(x->prob_out, x->prob);
	}
}

t_class *coin_class;

void *coin_new(t_symbol *s, int argc, t_atom *argv){
	t_coin *x = (t_coin *)pd_new(coin_class);
	x->prob = 50.0f;
	x->t_value = 1.0f;
	x->f_value = 0.0f;
	if (argc > 0) {
		t_float f = atom_getfloat(argv);
		if (f <= 0) {
			x->prob = 0.0f;
		}else if(f >= 100){
			x->prob = 100.0f;
		}else{
			x->prob = f;
		}
		switch (argc) {
			case 2:{
				x->t_value = atom_getfloat(argv+1);		
			}break;
			case 3:{
				x->t_value = atom_getfloat(argv+1);		
				x->f_value = atom_getfloat(argv+2);
			}break;
			default:
				break;
		}
	}
	inlet_new(&x->x_ob, &x->x_ob.ob_pd, gensym("float"), gensym("set"));
	x->x_outlet = outlet_new(&x->x_ob, gensym("float"));
	x->prob_out = outlet_new(&x->x_ob, gensym("float"));
	x->coin_out = outlet_new(&x->x_ob, gensym("float"));
	return (void *) x;
}

void coin_setup(void){
	coin_class = class_new(gensym("coin"), (t_newmethod)coin_new, 0, sizeof(t_coin), 0, A_GIMME, 0);
	class_addmethod(coin_class, (t_method)coin_set, gensym("set"), A_GIMME, 0);
	class_addbang(coin_class, coin_bang);
}



