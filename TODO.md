* Real unit tests for the runtime.
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
* There's a whole slew of stuff just begging for more shorthand.
* Are we rooting all the core classes correctly?
* ChimpModule?
* Arbitrary precision for ChimpInt.
* ChimpFloat?
* Cache bound methods on first access, or at object construction time if
  that makes more sense.
* Generational GC one day. Erlang-style GC algorithm switching another.

