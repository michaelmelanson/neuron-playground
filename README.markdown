
# Neuron model playground

This is an implementation of a Hindmarsh-Rose neuron model, with the
parameterization described in (Steur, 2006).

It uses the GNU Scientific Library for numeric integration, ZeroMQ for
internal messaging and pthread for threading.


## Usage

First, build the code:

> scons

Run a single-neuron simulation:

> ./neuron 1

The last command dumped a trace file in trace-1.txt, so let's plot it.

> gnuplot < plot.gnuplot

The included gnuplot script uses the Aqua terminal, so you'll have to
modify it if you're not on Mac OSX.