* Real unit tests for the runtime.
* Explicit mallocs for lwhashes. Should be managed by the GC.
* If a task creates a child task, I think (but haven't proven) that it's
  safe for the child to hold a reference to objects in the parent's GC,
  but not vice versa (i.e. values in new GCs may reference values in old GCs).
* There's a whole slew of stuff just begging for more shorthand.
* Are we rooting all the core classes correctly?
* ChimpModule?
* ChimpInt?
* Arbitrary precision for ChimpInt.
* ChimpFloat?
* Cache bound methods on first access, or at object construction time if
  that makes more sense.
* Explicit constructors?
* Generational GC one day. Erlang-style GC algorithm switching another.
* Syntax, one day.

