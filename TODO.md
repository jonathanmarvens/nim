* Real unit tests for the runtime.
* A GC occuring at the wrong time will have dire effects atm.
  Artificial stack frames / scopes are needed to make object lifetime explicit
* Lots of explicit malloc()ing where a slab allocator would probably do better
  (worst offender: ChimpRef).
* Classes defined in older GCs need to become sealed as soon as they are
  referenced from another GC. Make it so either implicitly or via an explicit
  API call. (e.g. chimp\_class\_freeze)
* There's a whole slew of stuff just begging for more shorthand.
* Are we rooting all the core classes correctly?
* ChimpModule? How can we make this play nice with our GC restrictions?
* ChimpInt?
* Arbitrary precision for ChimpInt.
* ChimpFloat?
* Cache bound methods on first access, or at object construction time if
  that makes more sense.
* Explicit constructors?
* Generational GC one day. Erlang-style GC algorithm switching another.
* Explicit GC parameter sucks. Implicit per-task GC should be fine.
* Syntax, one day.

