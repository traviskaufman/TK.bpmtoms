/********************************************************************
 *							TK.bpmtoms							    *
 * Takes beat value, number of steps, and bpm and turns it into a   *
 * floating-point value in milliseconds. Useful for tempo-synching  *
 * delays, LFOs, and any other object that takes a time-based value *
 *	*	*	*	*	*	*	*	*	*	*	*	*	*	*	*	*   *
 *							 UPDATES:								*
 * - 9/2/11: Fixed enum indexing (now starts at 1)					*
 * - 9/4/11: Added support for floating-point numbers				*
 ********************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include "ext.h"
#include "ext_obex.h"
#include "ext_strings.h"

static t_class *t_btm_class = NULL;

typedef enum _bV {
	q_Note = 1, e_Note, s_Note, t_Note
} BeatValue;

typedef struct _btm {
	t_object b_obj;
	double noteValue;
	double steps;
	double tempo;
	long m_in;
	void *m_proxy[1];
	void *m_outlet1;
} t_BPM;
	

void *BPM_new();
void BPM_free(t_BPM *x);
void BPM_int(t_BPM *x, long i);
void BPM_float(t_BPM *x, double f);
double BPM_calcMS(double *v, double *s, double *t);
void BPM_assist(t_BPM *x, void *b, long m, long a, char *s);

int main () {
	t_class *c = NULL;
	
	c = class_new("TK.bpmtoms", (method)BPM_new, (method)BPM_free, sizeof(t_BPM), 0L, 0);

	class_addmethod(c, (method)BPM_int, "int", A_LONG, 0);
	class_addmethod(c, (method)BPM_float, "float", A_FLOAT, 0);
	class_addmethod(c, (method)BPM_assist,"assist", A_CANT, 0);
	
	class_register(CLASS_BOX, c);
	
	t_btm_class = c;
	
	return EXIT_SUCCESS;
}

void *BPM_new() {
	int i;
	t_BPM *x = (t_BPM *)object_alloc(t_btm_class);
	x->noteValue = 0.25;
	x->steps = 1;
	x->tempo = 120;
	
	x->m_in = 3;
	for (i = 0; i < 2; i++)
		x->m_proxy[i] = proxy_new((t_object *)x, i+1, &x->m_in);
	x->m_outlet1 = intout((t_object *)x);
	return x;
}

void BPM_free(t_BPM *x) {
	if (x->noteValue)
		sysmem_freeptr(&x->noteValue);
	if (x->steps)
		sysmem_freeptr(&x->steps);
	if (x->tempo)
		sysmem_freeptr(&x->tempo);
	object_free(x->m_proxy);
}

void BPM_int(t_BPM *x, long i) {
	if (i > 0) {
		BeatValue nV = 1;
		switch (proxy_getinlet((t_object *)x)) {
			case 0:
				if (i > 0 && i <= 4) {
					nV = (short)i;
					switch (nV) {
						case q_Note:
							x->noteValue = 0.25; 
							break;
						case e_Note:
							x->noteValue = 0.125; 
							break;
						case s_Note:
							x->noteValue = 0.0625;
							break;
						case t_Note:
							x->noteValue = 0.03125;
							break;
					}
				}
				break;
			case 1:
				x->tempo = i;
				break;
			case 2: 
				x->steps = i;
				break;
			default:
				object_post((t_object *)x, "Unknown Error"); 
				break;
		}
	//post("nV = %g, x->steps = %i, x->tempo = %i", nV, x->steps, x->tempo); | for debugging
	outlet_float(x->m_outlet1, BPM_calcMS(&x->noteValue, &x->steps, &x->tempo));
	}
}

void BPM_float(t_BPM *x, double f) { // Essentially identical to BPM_int
	if (f > 0) {
		switch (proxy_getinlet((t_object *)x)) {
			case 0: // Allows user to specify custom beat value using a fraction (e.g. triplet = 1/3 = 0.333)
				if (f <= 1) { // Subdivision cannot be more than a whole note
					x->noteValue = f;
				}
				break;
			case 1:
				x->tempo = f;
				break;
			case 2:
				x->steps = f;
				break;
		}
	outlet_float(x->m_outlet1, BPM_calcMS(&x->noteValue, &x->steps, &x->tempo));
	}
}

void BPM_assist(t_BPM *x, void *b, long m, long a, char *s) {
	if (m == ASSIST_OUTLET)
		strncpy_zero(s, "Output (ms)", 512);
	else {
		switch (a) {
			case 0:
				strncpy_zero(s, "int (1-4) sets Q/E/S/T beat value, float (0-1) sets custom beat value", 512);
				break;
			case 1:
				strncpy_zero(s, "int/float sets number of steps", 512);
				break;
			case 2:
				strncpy_zero(s, "int/float sets tempo", 512);
				break;
		}
	}
}

double BPM_calcMS(double *v, double *s, double *t) { // Ms = ((BeatValue*Steps)*240000)/Tempo
	return (((*v) * (*s))*240000.0)/(*t);
}


