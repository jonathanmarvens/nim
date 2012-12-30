Chimp
=====

Chimp is a little dynamic, strongly typed programming language experiment
written in C. It has "shared nothing" tasks that communicate via message
passing and each task gets its own heap, garbage collector and virtual machine.

If you can imagine Rust or Erlang carrying Ruby & Python’s bastard surrogate
child... well, yeah. It’s kinda like that.

http://tomlee.co/2012/12/chimp-programming-language/

Running on OS X
---------------

Getting started with Chimp is a breeze.

    brew install pkg-config
    ./configure
    make
    ./chimp examples/helloworld.chimp

Running Tests
-------------

Unit testing uses the check library, so you'll need to install that before you
can run the tests:

    brew install check
    make clean check
