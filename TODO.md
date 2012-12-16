* More tests.
* Revisit the bytecode format.
  - Variable length instructions? 8-bit args are limited (also, 24-bit jumps :()
* Syntax for structured data. (class definitions?)
* Bitwise operators.
* Modulo operator.
* In-place operators for integers: +=, -=, /=, \*=, etc.
* Unary operators ++, --
* chimp_hash_get returning NULL and chimp\_nil completely sucks.
  Find a better way.
* Error handling.
* File I/O.
* Sockets.
* Expose methods on more classes.
* Rubyish symbols?
* Annotations?
* Immutable values ...
* Module definitions.
* Revisit my half-baked crappy memory management ideas: tasks, threads & GC.
  (i.e. do we care right now? probably not ...)
* VM stacks are made roots on VM creation, but are not removed from the root
  set at destruction. Either figure out a way to let VMs mark themselves, or
  provide a means of clearing elements from the root set.
* Explicit mallocs for lwhashes. Should be managed by the GC?
* Are we rooting all the core classes correctly?
* Arbitrary precision for ChimpInt.
* ChimpFloat?
* Hash & array literals containing only constants can be optimized at compile time.
* Unicode! (lol)
* Cache bound methods on first access, or at object construction time if
  that makes more sense. Er. Do we even need 'em at all?
  Implicit "self" on call?
* Generational GC one day. Erlang-style GC algorithm switching another.
