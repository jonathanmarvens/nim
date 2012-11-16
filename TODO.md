* More tests.
* Rewrite the GC (do away with copying collector for now?)
* Exceptions & exception handling.
* File I/O.
* Custom native types.
* A macro for safe assignment to object attributes with simultaneous
  allocation.
* Class definitions.
* Expose methods on more classes.
* Rubyish keyword args:
  foo(a, b, bar: 10, baz: 20)
* Rubyish symbols?
* Annotations?
* Revisit the bytecode format.
  - Variable length instructions? 8-bit args are limited (also, 24-bit jumps :()
  - MAKEARRAY for every call is probably slow/dumb.
* Immutable values ...
* Module definitions.
* Revisit my half-baked crappy memory management ideas: tasks, threads & GC.
  (i.e. do we care right now? probably not ...)
* Do away with explicit GC pointers. Implicit per-thread GC is always available.
* VM stacks are made roots on VM creation, but are not removed from the root
  set at destruction. Either figure out a way to let VMs mark themselves, or
  provide a means of clearing elements from the root set.
* Explicit mallocs for lwhashes. Should be managed by the GC?
* If a task creates a child task, I think (but haven't proven) that it's
  safe for the child to hold a reference to objects in the parent's GC,
  but not vice versa (i.e. values in new GCs may reference values in old GCs).
  XXX pretty sure this is dumb ^^
* Are we rooting all the core classes correctly?
* Arbitrary precision for ChimpInt.
* ChimpFloat?
* Hash & array literals containing only constants can be optimized at compile time.
* Unicode! (lol)
* Cache bound methods on first access, or at object construction time if
  that makes more sense. Er. Do we even need 'em at all?
  Implicit "self" on call?
* Generational GC one day. Erlang-style GC algorithm switching another.
