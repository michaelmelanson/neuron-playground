//
// Hindmarsh-Rose neuron model
//

#include <math.h>
#include <gsl/gsl_errno.h>
#include <gsl/gsl_matrix.h>
#include <gsl/gsl_odeiv.h>

#include "hindmarsh-rose.hh"


int hindmarsh_rose(double t, const double vars[], double funcs[],
		   void *paramsp)
{
  struct hindmarsh_rose_params_t params;
  params = *(struct hindmarsh_rose_params_t *) paramsp;

  double x      = vars[0];
  double y      = vars[1];
  double z      = vars[2];
  double epsp_I = vars[3];

  funcs[0] = (1/params.T_s) * (-params.a*x*x*x + params.b*x*x + y - z + params.I_t(t));
  funcs[1] = (1/params.T_s) * (params.c - params.d*x*x - params.beta*y);
  funcs[2] = ((1/params.T_s) * params.r * (params.s * (x - params.x_0) - z));

  for(unsigned int i = 0; i < NUM_EPSPS; ++i) {
    double a = 1.0 / 3.0;
    double dt = params.epsp[i] - t;

    double dirac = (params.epsp_amp / (a * sqrt(M_PI))) * exp(-(dt*dt) / (a*a));

    funcs[3] += -dirac / dt;
  }

  return GSL_SUCCESS;
}
