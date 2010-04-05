#ifndef HINDMARSH_ROSE_HH_INCLUDED
#define HINDMARSH_ROSE_HH_INCLUDED

#define NUM_EPSPS 10

//
// Parameters for the Hindmarsh-Rose model, as described in (Steur,
// 2006), p19.
//
struct hindmarsh_rose_params_t
{
  double a;	
  double b;	
  double c; 
  double d;
  double beta;
  double r;	
  double s;	
  double x_0;

  double T_s;
	
  double (*I_t)(double);

  double t_syn; // synaptic delay
  double epsp_amp; // EPSP amplitude
  double epsp[NUM_EPSPS]; // EPSP times
};

int hindmarsh_rose(double t, const double vars[], double funcs[],
		   void *paramsp);

#endif
