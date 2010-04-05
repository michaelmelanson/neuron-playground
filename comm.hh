#ifndef COMM_HPP_INCLUDED
#define COMM_HPP_INCLUDED


enum message_type_t {
  SPIKE,
  TRACE,

  STARTED,
  FINISHED,
  OVERLOAD
};

struct trace_message_t
{
  double v;	// membrane potential
  double na_k;	// sodium/potassium concentration
  double ions;  // various ion concentrations

  double epsp_I; // EPSP current
};

struct message_t
{
  char topic[9]; // topic for zeromq -- this must be the first field
  message_type_t type;

  int neuron_id;
  double time;

  union {
    struct trace_message_t trace;
  };
};


void send_spike_message(zmq::socket_t &sock, int neuron_id, double time);

void send_trace_message(zmq::socket_t &sock, int neuron_id, double time,
			double v, double na_k, double ions, double epsp_I);

void send_started_message(zmq::socket_t &sock, int neuron_id, double time);
void send_finished_message(zmq::socket_t &sock, int neuron_id, double time);
void send_overloaded_message(zmq::socket_t &sock, int neuron_id, double time);
#endif
