
#include <stdio.h>
#include <math.h>
#include <gsl/gsl_errno.h>
#include <gsl/gsl_matrix.h>
#include <gsl/gsl_odeiv.h>
#include <gsl/gsl_rng.h>
#include <zmq.hpp>
#include <pthread.h>
#include <sys/time.h>
#include <unistd.h>

#include "comm.hh"
#include "hindmarsh-rose.hh"

#define FORWARDER_INPUT_URL "inproc://input"
#define FORWARDER_OUTPUT_URL "inproc://output"


int jac (double t, const double y[], double *dfdy, 
	 double dfdt[], void *params)
{
  double mu = *(double *)params;
  gsl_matrix_view dfdy_mat = gsl_matrix_view_array (dfdy, 2, 2);
  gsl_matrix * m = &dfdy_mat.matrix; 
  gsl_matrix_set (m, 0, 0, 0.0);
  gsl_matrix_set (m, 0, 1, 1.0);
  gsl_matrix_set (m, 1, 0, -2.0*mu*y[0]*y[1] - 1.0);
  gsl_matrix_set (m, 1, 1, -mu*(y[0]*y[0] - 1.0));
  dfdt[0] = 0.0;
  dfdt[1] = 0.0;
  return GSL_SUCCESS;
}

void difftime(const struct timeval *start, const struct timeval *end,
              struct timeval *result)
{
   long sec;
   long usec;

   sec = end->tv_sec - start->tv_sec;
   usec = end->tv_usec - start->tv_usec;

   if (usec < 0)
   {
      sec -= 1;
      usec += 1000000;
   }

   result->tv_sec = sec;
   result->tv_usec = usec;
}

struct neuron_args_t
{
  int neuron_id;
  zmq::context_t *context;
};

double I_t(double t)
{
  if (t >= 100 && t <= 600) {
    return 0.75;
  } else if (t >= 1100 && t <= 1600) {
    return 1.0;
  } else {
    return 0.0;
  }
}

void *neuron_main(void *argsp)
{
  struct neuron_args_t args = *(struct neuron_args_t *) argsp;
  free(argsp);

  zmq::context_t *context = args.context;

  // send messages to the relay
  zmq::socket_t sock_out(*context, ZMQ_PUB);
  sock_out.connect(FORWARDER_INPUT_URL);

  // receive only spike messages from the relay
  zmq::socket_t sock_in(*context, ZMQ_SUB);
  sock_in.setsockopt(ZMQ_SUBSCRIBE, "spike", 5);
  sock_in.connect(FORWARDER_OUTPUT_URL);


#define DIMENSIONS 4
#define MAX_ERROR 1e-8
  const gsl_odeiv_step_type * T = gsl_odeiv_step_rk8pd;
  
  gsl_odeiv_step    *s = gsl_odeiv_step_alloc (T, DIMENSIONS);
  gsl_odeiv_control *c = gsl_odeiv_control_y_new (MAX_ERROR, 0.0);
  gsl_odeiv_evolve  *e = gsl_odeiv_evolve_alloc (DIMENSIONS);

  gsl_rng *r = gsl_rng_alloc(gsl_rng_default);
  
  struct hindmarsh_rose_params_t params;
  memset(&params, 0, sizeof(params));
  
  params.a	= 1;
  params.b	= 4;
  params.c	= 1;
  params.d	= 6;
  params.beta	= 1;
  params.r	= 0.01;
  params.s	= 1;
  params.x_0    = -1.5;
  params.T_s	= 1;
  params.I_t	= I_t;

  params.t_syn = 0.2;
  params.epsp_amp = 0.0; //1.0e-5;

  int epsp_index = 0;
  for(unsigned int i = 0; i < NUM_EPSPS; ++i) {
    params.epsp[i] = -100.0;
  }


  gsl_rng_free(r);

  gsl_odeiv_system sys = {hindmarsh_rose, jac, DIMENSIONS, &params};
     
  double t = -1000.0, t1 = 2000.0;
  double h = MAX_ERROR;
  double y[DIMENSIONS] = { -1, -13, -7, 0 };
  
  double last_t = t;

  
  int spiking = 0;
  int last_spiking = 0;


  send_started_message(sock_out, args.neuron_id, t);


  struct timeval start_time, now, elapsed;
  gettimeofday(&start_time, NULL);


  while (t < t1) {

    for(;;) {      
      zmq_pollitem_t items[1];
      items[0].socket = sock_in;
      items[0].events = ZMQ_POLLIN;
      int rc = zmq::poll(items, 1, 0);

      if (rc < 0) {
	perror("zmq_poll");
      } else if (rc == 1) {
	zmq::message_t msg;
	sock_in.recv(&msg);
    
	struct message_t *message = (struct message_t *) msg.data();
	assert(message->type == SPIKE);

	if (message->neuron_id != args.neuron_id) {
	  params.epsp[epsp_index] = message->time + params.t_syn;
	  epsp_index = (epsp_index + 1) % NUM_EPSPS;
	}
      } else {
	// no messages
	break;
      }
    }

#ifdef REALTIME
#define REALTIME_FACTOR 20
    // maintain real-time

    gettimeofday(&now, NULL);
    difftime(&start_time, &now, &elapsed);
    
    unsigned long sim_elapsed = (unsigned long) (t*1000.0);
    unsigned long real_elapsed = elapsed.tv_sec*1000000 + elapsed.tv_usec;
    real_elapsed /= REALTIME_FACTOR;

    if (real_elapsed < sim_elapsed) {
      unsigned long diff = sim_elapsed - real_elapsed;
      usleep(diff * REALTIME_FACTOR);
    } else if (sim_elapsed > 0 && (real_elapsed - sim_elapsed > 100000)) {
      send_overloaded_message(sock_out, args.neuron_id, t);
    }
#endif


    last_t = t;
    last_spiking = spiking;

    int status = gsl_odeiv_evolve_apply (e, c, s,
					 &sys, 
					 &t, t1,
					 &h, y);
     
    if (status != GSL_SUCCESS)
      break;

    send_trace_message(sock_out, args.neuron_id, t, y[0], y[1], y[2], y[3]);

    spiking = (y[1] < -3.5);
    if (spiking && !last_spiking) {
      send_spike_message(sock_out, args.neuron_id, t);
    }
  }

  gsl_odeiv_evolve_free (e);
  gsl_odeiv_control_free (c);
  gsl_odeiv_step_free (s);


  send_finished_message(sock_out, args.neuron_id, t);
  return (void *) 0;
}


// simple, generic forwarding device for inter-neuron signalling
struct forwarder_config_t
{
  zmq::context_t *context;
  char *input_url; // url to accept messages on
  char *output_url; // url to send messages on
};

void *forwarder_main(void *configp)
{
  struct forwarder_config_t config = *(struct forwarder_config_t *) configp;
  free(configp);

  zmq::context_t *ctx = (zmq::context_t *) config.context;
  zmq::socket_t sock_in(*ctx, ZMQ_SUB);
  sock_in.setsockopt(ZMQ_SUBSCRIBE, "", 0);
  sock_in.bind(config.input_url);

  zmq::socket_t sock_out(*ctx, ZMQ_PUB);
  sock_out.bind(config.output_url);


  for(;;) {
    zmq::message_t msg;

    sock_in.recv(&msg);
    sock_out.send(msg);
  }
}


void *logger_main(void *ctxp)
{
  zmq::context_t *ctx = (zmq::context_t *) ctxp;
  zmq::socket_t sock(*ctx, ZMQ_SUB);
  sock.setsockopt(ZMQ_SUBSCRIBE, "", 0);
  sock.connect(FORWARDER_OUTPUT_URL);

  int started = 0;
  int alive_neurons = 0;

  int num_neurons = 0;
  FILE **files = NULL;

  while(!started || alive_neurons > 0) {
    zmq::message_t msg;
    sock.recv(&msg);
    
    struct message_t *message = (struct message_t *) msg.data();

    if (message->neuron_id > num_neurons) {
      int old_num_neurons = num_neurons;
      num_neurons = message->neuron_id;
      files = (FILE **) realloc(files, sizeof(FILE*) * (num_neurons+1));

      for (int i = old_num_neurons; i <= num_neurons; ++i) {
	files[i] = NULL;
      }
    }

    char tracepath[100];
    if (files[message->neuron_id] == NULL) {
      sprintf(tracepath, "trace-%d.txt", message->neuron_id);
      files[message->neuron_id] = fopen(tracepath, "a");
    }
   
    switch(message->type) {
    case SPIKE:
      //printf("%.8f: neuron %d spiked\n",
      //message->time, message->neuron_id);

      fprintf(files[message->neuron_id], "# spike at t=%.8f\n", message->time);
      break;

    case STARTED:
      started = 1;
      ++alive_neurons;

      printf("neuron %d has started\n", message->neuron_id);

      // this is an exception -- wipe any existing trace
      fclose(files[message->neuron_id]);
      files[message->neuron_id] = fopen(tracepath, "w");
      fprintf(files[message->neuron_id],
	      "# trace started at time %.8f\n", message->time);
      break;

    case FINISHED:
      --alive_neurons;

      printf("neuron %d has announced it's finished\n", message->neuron_id);
      fprintf(files[message->neuron_id],
	      "# neuron finished at time %.8f\n", message->time);
      break;

    case OVERLOAD:
      printf("neuron %d is overloaded\n", message->neuron_id);
      break;

    case TRACE:
      fprintf(files[message->neuron_id],
	      "%.8f %.8f %.8f %.8f %.8f\n",
	      message->time, message->trace.v, message->trace.na_k,
	      message->trace.ions, message->trace.epsp_I);
      break;

    default:
      printf("Received unknown message type %d\n", message->type);
      break;
    }
  }

  for (unsigned int i = 0; i < num_neurons; ++i) {
    if (files[i] != NULL) {
      fclose(files[i]);
    }
  }
  free(files);

  return NULL;
}

int main(int argc, char **argv)
{
  int num_neurons = atoi(argv[1]);

  zmq::context_t ctx(num_neurons+2, 3, ZMQ_POLL);

  gsl_rng_env_setup();

  pthread_t forwarder_thread;
  struct forwarder_config_t *forwarder_config =
    (struct forwarder_config_t *) malloc(sizeof(struct forwarder_config_t));
  
  forwarder_config->context = &ctx;
  forwarder_config->input_url = FORWARDER_INPUT_URL;
  forwarder_config->output_url = FORWARDER_OUTPUT_URL;

  pthread_create(&forwarder_thread, NULL,
		 forwarder_main, forwarder_config);


  usleep(10000);

  pthread_t logger_thread;
  pthread_create(&logger_thread, NULL, logger_main, &ctx);

  pthread_t neuron_threads[num_neurons];

  for (int i = 0; i < num_neurons; ++i) {
    pthread_t neuron_thread;
    struct neuron_args_t *args =
      (struct neuron_args_t *) malloc(sizeof(struct neuron_args_t));
    args->neuron_id = i+1;
    args->context = &ctx;

    pthread_create(&neuron_threads[i], NULL, neuron_main, args);
  }

  for (int i = 0; i < num_neurons; ++i) {
    pthread_join(neuron_threads[i], NULL);
  }
  
  pthread_join(logger_thread, NULL);
  pthread_cancel(forwarder_thread);

  return 0;
}
