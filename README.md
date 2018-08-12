# dsarch

A C++ library for tracking network cost of simulated distributed algorithms.

Using this library, one can implement distributed algorithms at a high level and
simulate them while tracking communication activity.

## Compiling and installing

This code depends on the following packages:

- BOOST for various small things
- cxxtest for unit testing
- doxygen for documentation

To compile and install, the GNU autotools suite is used. Do the following:
> autoreconf -ivf
> ./configure --enable-debug
> make 

To run unit tests, run
> make check

To create the documentation, do
> make doxygen-doc

