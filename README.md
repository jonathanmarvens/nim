Nim
=====

Nim is a little dynamic, strongly typed programming language experiment
written in C. It has "shared nothing" tasks that communicate via message
passing and each task gets its own heap, garbage collector and virtual machine.

If you can imagine Rust or Erlang carrying Ruby & Python’s bastard surrogate
child... well, yeah. It’s kinda like that.

Nim was formerly known as 'chimp'. An introductory blog post is available here:

http://tomlee.co/2012/12/chimp-programming-language/

Running on OS X
---------------

Getting started with Nim is a breeze.

    brew install cmake
    cmake .
    make
    ./nim examples/helloworld.nim

Other things you might need to install.

    brew install check
    brew install valgrind

Running Tests
-------------

Unit testing are written partially with the check library (for testing C
and partially in NimUnit) If you haven't already installed check, you might
need to do that before you run the tests:

    cmake .
    make clean test
