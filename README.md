# dataflow

This repo is the very early experimental code that will eventually become a C++11 library that implements a dataflow model on a directed acyclic graph (DAG).

The idea is inspired by various financial software systems that have this use case e.g. asset pricing models defined by a graph of spreads to other assets.

The aims of the library are to :

1) Provide a framework to construct a graph of dependent values (i.e. nodes) defined by the arguments and the return values of pure functions defined by the client
2) Provide a singlethreaded and multithreaded routine to calculate all the values in the graph in the correct order of dependency
3) Avoid recomputing values that have not changed
4) To allow the graph to be updated in between calculations
5) Be statically type-safe throughout
6) To be thread-safe to the extent that client code can call the library on multiple threads.
6) Strive for simplicity of use - Not necessarily of implementation
7) Minimal dependencies
8) Header only library
