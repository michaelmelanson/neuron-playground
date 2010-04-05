#include <zmq.hpp>
#include "comm.hh"


void send_spike_message(zmq::socket_t &sock, int neuron_id, double time)
{
  struct message_t message;
  memset(&message, 0, sizeof(struct message_t));
  strcpy(message.topic, "spike");
  message.type = SPIKE;
  message.neuron_id = neuron_id;
  message.time = time;

  zmq::message_t msg(sizeof(struct message_t));
  memcpy(msg.data(), &message, sizeof(struct message_t));

  assert(sock.send(msg));
}

void send_trace_message(zmq::socket_t &sock, int neuron_id, double time,
			double v, double na_k, double ions, double epsp_I)
{
  struct message_t message;
  memset(&message, 0, sizeof(struct message_t));
  strcpy(message.topic, "trace");
  message.type = TRACE;
  message.neuron_id = neuron_id;
  message.time = time;
  message.trace.v = v;
  message.trace.na_k = na_k;
  message.trace.ions = ions;
  message.trace.epsp_I = epsp_I;

  zmq::message_t msg(sizeof(struct message_t));
  memcpy(msg.data(), &message, sizeof(struct message_t));

  assert(sock.send(msg));
}

void send_started_message(zmq::socket_t &sock, int neuron_id, double time)
{
  struct message_t message;
  memset(&message, 0, sizeof(struct message_t));
  strcpy(message.topic, "start");
  message.type = STARTED;
  message.neuron_id = neuron_id;
  message.time = time;

  zmq::message_t msg(sizeof(struct message_t));
  memcpy(msg.data(), &message, sizeof(struct message_t));

  assert(sock.send(msg));
}

void send_finished_message(zmq::socket_t &sock, int neuron_id, double time)
{
  struct message_t message;
  memset(&message, 0, sizeof(struct message_t));
  strcpy(message.topic, "finish");
  message.type = FINISHED;
  message.neuron_id = neuron_id;
  message.time = time;

  zmq::message_t msg(sizeof(struct message_t));
  memcpy(msg.data(), &message, sizeof(struct message_t));

  assert(sock.send(msg));
}

void send_overloaded_message(zmq::socket_t &sock, int neuron_id, double time)
{
  struct message_t message;
  memset(&message, 0, sizeof(struct message_t));
  strcpy(message.topic, "overload");
  message.type = OVERLOAD;
  message.neuron_id = neuron_id;
  message.time = time;
  
  zmq::message_t msg(sizeof(struct message_t));
  memcpy(msg.data(), &message, sizeof(struct message_t));
  
  sock.send(msg);
}
