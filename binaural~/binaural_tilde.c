//
//  binaural_tilde.c
//  binaural
//
//  Created by Cheng-i Wang on 10/31/13.
//  Copyright (c) 2013 __MyCompanyName__. All rights reserved.
//

#include "m_pd.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "include/mit_hrtf_lib.h"
#ifdef NT
#pragma warning( disable : 4244 )
#pragma warning( disable : 4305 )
#endif

#define HOPSIZE 0.5f
#define CONVLEN 128
#define NORM 0.00003051757
#define SR 44100
#define I_CONVLEN 0.0078125f

static t_class *binaural_class;

typedef struct _binaural{
	t_object x_obj;
	t_float x_f;
	t_float x_azimuth;
	t_float x_azi_p;
	t_float x_elevation;
	t_float x_ele_p;
	t_float b_diffuse;
	short **x_imp_a;
	short **x_imp_b;
	short **x_imp_c;
	short **x_imp_d;
	short **x_impulse;
	short **x_impulse_prev;
	t_float *x_buffer;
	t_int **neighbor;
}t_binaural;

//	Interpolation between spatial position

float find_azimuth_increment(int e){
	float a_inc = 0.0f;
	
	switch(e)
	{
		case 0:		
			a_inc = 180.f / (MIT_HRTF_AZI_POSITIONS_00 - 1);		
			break;	// 180 5
		case 10:	
		case -10:	
			a_inc = 180.f / (MIT_HRTF_AZI_POSITIONS_10 - 1);		
			break;	// 180 5
		case 20:	
		case -20:	
			a_inc = 180.f / (MIT_HRTF_AZI_POSITIONS_20 - 1);		
			break;	// 180 5
		case 30:	
		case -30:	
			a_inc = 180.f / (MIT_HRTF_AZI_POSITIONS_30 - 1);		
			break;	// 180 6
		case 40:	
		case -40:	
			a_inc = 180.f / (MIT_HRTF_AZI_POSITIONS_40 - 1);		
			break;	// 180 6.43
		case 50:	
			a_inc = 176.f / (MIT_HRTF_AZI_POSITIONS_50 - 1);		
			break;	// 176 8
		case 60:	
			a_inc = 180.f / (MIT_HRTF_AZI_POSITIONS_60 - 1);		
			break;	// 180 10
		case 70:	
			a_inc = 180.f / (MIT_HRTF_AZI_POSITIONS_70 - 1);		
			break;	// 180 15
		case 80:	
			a_inc = 180.f / (MIT_HRTF_AZI_POSITIONS_80 - 1);		
			break;	// 180 30
		case 90:	
			a_inc = 0.f;												
			break;	// 0   1
	};
	
	return a_inc;
}

void find_neighbor(t_binaural *x){
	// The code in this function is adapted from the mit_hrtf_lib.
	
	int _e = (int)x->x_elevation;
	
	if (_e > 90){
		_e = 90;
	}else if (_e < -40){
		_e = -40;
	}
	
	int _a = (int)x->x_azimuth;
	
	if (_a > 180){
		_a = 180;
	}else if (_a < -180){
		_a = -180;
	}
	
	float a_inc_1 = 0.0f;
	float a_inc_2 = 0.0f;
	
	if(_e < 0){
		
		x->neighbor[0][0] = (_e/10) * 10;
		x->neighbor[1][0] = x->neighbor[0][0];
		
		if (!(_e % 10)){
			x->neighbor[2][0] = x->neighbor[0][0];			
		}else {
			x->neighbor[2][0] = x->neighbor[0][0] - 10;
		}
		
		x->neighbor[3][0] = x->neighbor[2][0];
	}else{
		x->neighbor[2][0] = (_e/10) * 10;
		x->neighbor[3][0] = x->neighbor[2][0];
		
		if (!(_e % 10)){
			x->neighbor[0][0] = x->neighbor[2][0];			
		}else {
			x->neighbor[0][0] = x->neighbor[2][0] + 10;
		}
		x->neighbor[1][0] = x->neighbor[0][0];
	}
	
	// Elevation of 50 has a maximum 176 in the azimuth plane so we need to handle that.
	if(_e == 50){
		if(_a < 0){
			_a = _a < -176 ? -176 : _a;
		}else{
			_a = _a > 176 ? 176 : _a;			
		}
	}

	a_inc_1 = find_azimuth_increment(x->neighbor[0][0]);
	a_inc_2 = find_azimuth_increment(x->neighbor[2][0]);
	
	if(_a < 0){
		x->neighbor[0][1] = (int)(_a/a_inc_1)*a_inc_1;
		
		if (a_inc_1 == 0) {
			x->neighbor[1][1] = 0;
			x->neighbor[0][1] = 0; 
		}else {
			x->neighbor[1][1] = (int)(_a/a_inc_1)*a_inc_1;
			if (!(_a % (int)a_inc_1)){
				x->neighbor[0][1] = x->neighbor[1][1];			
			}else {
				x->neighbor[0][1] = x->neighbor[1][1] - a_inc_1;
			}
		}
		
		if (a_inc_2 == 0) {
			x->neighbor[3][1] = 0;
			x->neighbor[2][1] = 0;
		}else {
			x->neighbor[3][1] = (int)(_a/a_inc_2)*a_inc_2;
			if (!(_a % (int)a_inc_2)){
				x->neighbor[2][1] = x->neighbor[3][1];			
			}else {
				x->neighbor[2][1] = x->neighbor[3][1] - a_inc_2;
			}
		}
	}else if (_a == 0) {
		x->neighbor[0][1] = 0;
		x->neighbor[1][1] = 0;
		x->neighbor[2][1] = 0;
		x->neighbor[3][1] = 0;	
	}else{
		
		if (a_inc_1 == 0) {
			x->neighbor[1][1] = 0;
			x->neighbor[0][1] = 0; 
		}else {
			x->neighbor[1][1] = (int)(_a/a_inc_1)*a_inc_1;
			if (!(_a % (int)a_inc_1)){
				x->neighbor[0][1] = x->neighbor[1][1];			
			}else {
				x->neighbor[0][1] = x->neighbor[1][1] + a_inc_1;
			}
		}
		
		if (a_inc_2 == 0) {
			x->neighbor[3][1] = 0;
			x->neighbor[2][1] = 0;
		}else {
			x->neighbor[3][1] = (int)(_a/a_inc_2)*a_inc_2;
			if (!(_a % (int)a_inc_2)){
				x->neighbor[2][1] = x->neighbor[3][1];			
			}else {
				x->neighbor[2][1] = x->neighbor[3][1] + a_inc_2;
			}
		}		
	}
}

void spatial_interpolate(t_binaural *x){
	
	find_neighbor(x);
	
	x->b_diffuse = (x->b_diffuse > 0) ? 1.f:x->b_diffuse;
	x->b_diffuse = (x->b_diffuse < 0) ? 0.f:x->b_diffuse;
	
	if (x->neighbor[0][0] == x->neighbor[2][0]) {
		if (x->neighbor[0][1] == x->neighbor[1][1]) {
			mit_hrtf_get((int *)&x->neighbor[0][1],(int *)&x->neighbor[0][0], SR, x->b_diffuse, x->x_impulse[0], x->x_impulse[1]);
		}else {
			float azi_diff_ab = (float)x->neighbor[1][1]-(float)x->neighbor[0][1];
			azi_diff_ab	= (azi_diff_ab < 0) ? -1.f*azi_diff_ab:azi_diff_ab;
			float azi_a = x->x_azimuth - (float)x->neighbor[0][1];
			azi_a = (azi_a < 0) ? -1.f*azi_a:azi_a;
			azi_a = 1.f - azi_a/azi_diff_ab;
			float azi_b = 1.f - azi_a;
			mit_hrtf_get((int *)&x->neighbor[0][1], (int *)&x->neighbor[0][0], SR, x->b_diffuse, x->x_imp_a[0], x->x_imp_a[1]);
			mit_hrtf_get((int *)&x->neighbor[1][1], (int *)&x->neighbor[1][0], SR, x->b_diffuse, x->x_imp_b[0], x->x_imp_b[1]);
			for (int i = 0; i < CONVLEN; i++) {
				x->x_impulse[0][i] = (azi_a * x->x_imp_a[0][i] + azi_b * x->x_imp_b[0][i]);
				x->x_impulse[1][i] = (azi_a * x->x_imp_a[1][i] + azi_b * x->x_imp_b[1][i]);
			}
		}
	}else {
		float ele_diff = (float)x->neighbor[2][0]-(float)x->neighbor[0][0];
		float ele_ab = 1.f - (x->x_elevation - (float)x->neighbor[0][0])/ele_diff;
		float ele_cd = 1.f - ele_ab;
		if (x->neighbor[0][1] == x->neighbor[1][1]) {
			mit_hrtf_get((int *)&x->neighbor[0][1],(int *)&x->neighbor[0][0], SR, x->b_diffuse, x->x_imp_a[0], x->x_imp_a[1]);
			mit_hrtf_get((int *)&x->neighbor[2][1],(int *)&x->neighbor[2][0], SR, x->b_diffuse, x->x_imp_c[0], x->x_imp_c[1]);
			for (int i = 0; i < CONVLEN; i++) {
				x->x_impulse[0][i] = (ele_ab * x->x_imp_a[0][i] + ele_cd * x->x_imp_c[0][i]);
				x->x_impulse[1][i] = (ele_ab * x->x_imp_a[1][i] + ele_cd * x->x_imp_c[1][i]);
			}
		}else {
			if (x->neighbor[1][1]==x->neighbor[0][1]) {
				float azi_diff_cd = (float)x->neighbor[3][1]-(float)x->neighbor[2][1];
				azi_diff_cd	= (azi_diff_cd < 0) ? -1.f*azi_diff_cd:azi_diff_cd;
				float azi_c = x->x_azimuth - (float)x->neighbor[2][1];
				azi_c = (azi_c < 0) ? -1.f*azi_c:azi_c;
				azi_c = 1.f - azi_c/azi_diff_cd;
				float azi_d = 1.f - azi_c;
				mit_hrtf_get((int *)&x->neighbor[0][1], (int *)&x->neighbor[0][0], SR, x->b_diffuse, x->x_imp_a[0], x->x_imp_a[1]);
				mit_hrtf_get((int *)&x->neighbor[2][1], (int *)&x->neighbor[2][0], SR, x->b_diffuse, x->x_imp_c[0], x->x_imp_c[1]);
				mit_hrtf_get((int *)&x->neighbor[3][1], (int *)&x->neighbor[3][0], SR, x->b_diffuse, x->x_imp_d[0], x->x_imp_d[1]);
				for (int i = 0; i < CONVLEN; i++) {
					x->x_impulse[0][i] = ele_ab*x->x_imp_a[0][i] + ele_cd*(azi_c * x->x_imp_c[0][i] + azi_d * x->x_imp_d[0][i]);
					x->x_impulse[1][i] = ele_ab*x->x_imp_a[1][i] + ele_cd*(azi_c * x->x_imp_c[1][i] + azi_d * x->x_imp_d[1][i]);
				}
			}else if (x->neighbor[3][1]==x->neighbor[2][1]) {
				float azi_diff_ab = (float)x->neighbor[1][1]-(float)x->neighbor[0][1];
				azi_diff_ab	= (azi_diff_ab < 0) ? -1.f*azi_diff_ab:azi_diff_ab;
				float azi_a = x->x_azimuth - (float)x->neighbor[0][1];
				azi_a = (azi_a < 0) ? -1.f*azi_a:azi_a;
				azi_a = 1.f - azi_a/azi_diff_ab;
				float azi_b = 1.f - azi_a;
				mit_hrtf_get((int *)&x->neighbor[0][1], (int *)&x->neighbor[0][0], SR, x->b_diffuse, x->x_imp_a[0], x->x_imp_a[1]);
				mit_hrtf_get((int *)&x->neighbor[1][1], (int *)&x->neighbor[1][0], SR, x->b_diffuse, x->x_imp_b[0], x->x_imp_b[1]);
				mit_hrtf_get((int *)&x->neighbor[2][1], (int *)&x->neighbor[2][0], SR, x->b_diffuse, x->x_imp_c[0], x->x_imp_c[1]);
				for (int i = 0; i < CONVLEN; i++) {
					x->x_impulse[0][i] = ele_ab*(azi_a * x->x_imp_a[0][i] + azi_b * x->x_imp_b[0][i])+ ele_cd * x->x_imp_c[0][i];
					x->x_impulse[1][i] = ele_ab*(azi_a * x->x_imp_a[1][i] + azi_b * x->x_imp_b[1][i])+ ele_cd * x->x_imp_c[1][i];
				}
			}else {
				float azi_diff_ab = (float)x->neighbor[1][1]-(float)x->neighbor[0][1];
				azi_diff_ab	= (azi_diff_ab < 0) ? -1.f*azi_diff_ab:azi_diff_ab;
				float azi_a = x->x_azimuth - (float)x->neighbor[0][1];
				azi_a = (azi_a < 0) ? -1.f*azi_a:azi_a;
				azi_a = 1.f - azi_a/azi_diff_ab;
				float azi_b = 1.f - azi_a;
				float azi_diff_cd = (float)x->neighbor[3][1]-(float)x->neighbor[2][1];
				azi_diff_cd	= (azi_diff_cd < 0) ? -1.f*azi_diff_cd:azi_diff_cd;
				float azi_c = x->x_azimuth - (float)x->neighbor[2][1];
				azi_c = (azi_c < 0) ? -1.f*azi_c:azi_c;
				azi_c = 1.f - azi_c/azi_diff_cd;
				float azi_d = 1.f - azi_c;
				mit_hrtf_get((int *)&x->neighbor[0][1], (int *)&x->neighbor[0][0], SR, x->b_diffuse, x->x_imp_a[0], x->x_imp_a[1]);
				mit_hrtf_get((int *)&x->neighbor[1][1], (int *)&x->neighbor[1][0], SR, x->b_diffuse, x->x_imp_b[0], x->x_imp_b[1]);
				mit_hrtf_get((int *)&x->neighbor[2][1], (int *)&x->neighbor[2][0], SR, x->b_diffuse, x->x_imp_c[0], x->x_imp_c[1]);
				mit_hrtf_get((int *)&x->neighbor[3][1], (int *)&x->neighbor[3][0], SR, x->b_diffuse, x->x_imp_d[0], x->x_imp_d[1]);
				
				for (int i = 0; i < CONVLEN; i++) {
					x->x_impulse[0][i] = ele_ab*(azi_a * x->x_imp_a[0][i] + azi_b * x->x_imp_b[0][i])+ ele_cd*(azi_c * x->x_imp_c[0][i] + azi_d * x->x_imp_d[0][i]);
					x->x_impulse[1][i] = ele_ab*(azi_a * x->x_imp_a[1][i] + azi_b * x->x_imp_b[1][i])+ ele_cd*(azi_c * x->x_imp_c[1][i] + azi_d * x->x_imp_d[1][i]);
				}
			}
		}
	}
}

void shift_buffer(t_binaural *x, t_float n){
	
	for (int i = CONVLEN - 1; i > 0 ; i--) {
		x->x_buffer[i] = x->x_buffer[i-1];
	}
	x->x_buffer[0] = n;
}

t_float convolution(short *impulse, short *impulse_prev, t_float *buffer){
	t_float out = 0.f;

	for (int i = 0; i < CONVLEN; i++) {
		out = out + (((t_float)impulse_prev[i]*(CONVLEN-i)+(t_float)impulse[i]*i)*I_CONVLEN*NORM) * buffer[i];
		impulse_prev[i] = impulse[i];
	}
	return out;
}

static t_int *binaural_perform(t_int *w){
		
	t_binaural *x = (t_binaural *)(w[1]);
	t_float *in = (t_float *)(w[2]);
	t_float *r_out = (t_float *)(w[3]);
	t_float *l_out = (t_float *)(w[4]);
	t_int n = (t_int)(w[5]);
	t_float _n = 1.f/(t_float)n;
	t_float azi_d = (x->x_azimuth - x->x_azi_p)*_n;
	t_float ele_d = (x->x_elevation - x->x_ele_p)*_n;
	int frame = 0;
	
	while (n--){
		x->x_azimuth = x->x_azi_p + azi_d;
		x->x_elevation = x->x_ele_p + ele_d;
		spatial_interpolate(x);
		x->x_azi_p = x->x_azi_p + azi_d;
		x->x_ele_p = x->x_ele_p + ele_d;
		shift_buffer(x, in[frame]);
		r_out[frame] = convolution(x->x_impulse[1], x->x_impulse_prev[1], x->x_buffer);
		l_out[frame] = convolution(x->x_impulse[0], x->x_impulse_prev[1], x->x_buffer);
		frame ++;
	}	
	return (w+6);
}


static void binaural_dsp(t_binaural *x, t_signal **sp){
	dsp_add(binaural_perform, 5, x, sp[0]->s_vec, sp[1]->s_vec, sp[2]->s_vec, sp[0]->s_n);
}

static void *binaural_new(t_symbol *s, int argc, t_atom *argv){
	t_binaural *x = (t_binaural *)pd_new(binaural_class);
	floatinlet_new(&x->x_obj, &x->b_diffuse);
	floatinlet_new(&x->x_obj, &x->x_elevation);
	floatinlet_new(&x->x_obj, &x->x_azimuth);
	outlet_new(&x->x_obj, &s_signal);
	outlet_new(&x->x_obj, &s_signal);

	x->b_diffuse = 1;
	x->x_elevation = 0.0f;	x->x_ele_p = 0.0f;
	x->x_azimuth = 0.0f;	x->x_azi_p = 0.0f;
	
	switch (argc) {
		case 1:
			x->b_diffuse = atom_getfloat(argv);
			break;
		case 2:
			x->b_diffuse = atom_getfloat(argv);
			x->x_elevation = atom_getfloat(argv+1);	x->x_ele_p = x->x_elevation;
			break;
		case 3:
			x->b_diffuse = atom_getfloat(argv);
			x->x_elevation = atom_getfloat(argv+1);	x->x_ele_p = x->x_elevation;
			x->x_azimuth = atom_getfloat(argv+2);	x->x_azi_p = x->x_azimuth;
			break;
		default:
			break;
	}
	
	x->x_impulse = (short **)malloc(2 * sizeof(short *));
	x->x_impulse[0] = (short *)malloc(CONVLEN* sizeof(short));	x->x_impulse[1] = (short *)malloc(CONVLEN* sizeof(short));
	x->x_impulse_prev = (short **)malloc(2 * sizeof(short *));
	x->x_impulse_prev[0] = (short *)malloc(CONVLEN* sizeof(short));	x->x_impulse_prev[1] = (short *)malloc(CONVLEN* sizeof(short));
	x->x_buffer = (t_float *)malloc(CONVLEN* sizeof(t_float));
	memset(x->x_buffer, 0, CONVLEN * sizeof(t_float));
	x->x_imp_a = (short **)malloc(2 * sizeof(short *));
	x->x_imp_a[0] = (short *)malloc(CONVLEN * sizeof(short));	x->x_imp_a[1] = (short *)malloc(CONVLEN * sizeof(short));
	mit_hrtf_get((int *)&x->x_azimuth, (int *)&x->x_elevation, SR, x->b_diffuse, x->x_imp_a[0], x->x_imp_a[1]);
	x->x_imp_b = (short **)malloc(2 * sizeof(short *));
	x->x_imp_b[0] = (short *)malloc(CONVLEN * sizeof(short));	x->x_imp_b[1] = (short *)malloc(CONVLEN * sizeof(short));
	mit_hrtf_get((int *)&x->x_azimuth, (int *)&x->x_elevation, SR, x->b_diffuse, x->x_imp_b[0], x->x_imp_b[1]);
	x->x_imp_c = (short **)malloc(2 * sizeof(short *));
	x->x_imp_c[0] = (short *)malloc(CONVLEN * sizeof(short));	x->x_imp_c[1] = (short *)malloc(CONVLEN * sizeof(short));
	mit_hrtf_get((int *)&x->x_azimuth, (int *)&x->x_elevation, SR, x->b_diffuse, x->x_imp_c[0], x->x_imp_c[1]);
	x->x_imp_d = (short **)malloc(2 * sizeof(short *));
	x->x_imp_d[0] = (short *)malloc(CONVLEN * sizeof(short));	x->x_imp_d[1] = (short *)malloc(CONVLEN * sizeof(short));
	mit_hrtf_get((int *)&x->x_azimuth, (int *)&x->x_elevation, SR, x->b_diffuse, x->x_imp_d[0], x->x_imp_d[1]);
	x->neighbor = (t_int **)malloc(4 * sizeof(t_int *));
	x->neighbor[0] = (t_int *)malloc(2 * sizeof(t_int));	x->neighbor[1] = (t_int *)malloc(2 * sizeof(t_int));
	x->neighbor[2] = (t_int *)malloc(2 * sizeof(t_int));	x->neighbor[3] = (t_int *)malloc(2 * sizeof(t_int));
	
	for (int i = 0; i < 4; i++) {
		memset(x->neighbor[i], 0, 2 * sizeof(t_int));
	}	
	return x;
}

static void binaural_free(t_binaural *x){
	
	if (x->x_imp_a != 0) {
		free(x->x_imp_a);
		x->x_imp_a = 0;
	}
	if (x->x_imp_b != 0) {
		free(x->x_imp_b);
		x->x_imp_b = 0;
	}
	if (x->x_imp_c != 0) {
		free(x->x_imp_c);
		x->x_imp_c = 0;
	}
	if (x->x_imp_d != 0) {
		free(x->x_imp_d);
		x->x_imp_d = 0;
	}
	if (x->neighbor != 0) {
		free(x->neighbor);
		x->neighbor = 0;
	}
	if (x->x_buffer != 0) {
		free(x->x_buffer);
		x->x_buffer = 0;
	}
	if (x->x_impulse != 0) {
		free(x->x_impulse);
		x->x_impulse = 0;
	}
	if (x->x_impulse_prev != 0) {
		free(x->x_impulse_prev);
		x->x_impulse_prev = 0;
	}
	
}

void binaural_tilde_setup(void){
	binaural_class = class_new(gensym("binaural~"), 
							   (t_newmethod)binaural_new, 
							   (t_method)binaural_free, 
							   sizeof(t_binaural), 0, A_GIMME, 0);
	CLASS_MAINSIGNALIN(binaural_class, t_binaural, x_f);
	class_addmethod(binaural_class, (t_method)binaural_dsp, gensym("dsp"), 0);
}