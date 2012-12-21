* Add the `self` keyword.
* Hash matching.
* Error handling.
* Unit testing API.
* More tests.
* Less semicolons.
* Revisit the bytecode format.
  - Variable length instructions? 8-bit args are limited (also, 24-bit jumps :()
* The task API is sucky and inconsistent with the rest of the codebase
  (sometimes ChimpTaskInternal, sometimes ChimpTask)
* inbox/outbox terminology for tasks.
  (inbox/outbox is always from the perspective of the 'local' task, and
   reversed for 'remote' tasks).
* Syntax for structured data. (class definitions?)
* Bitwise operators.
* Modulo operator.
* In-place operators for integers: +=, -=, /=, \*=, etc.
* Unary operators ++, --
* chimp_hash_get returning NULL and chimp\_nil completely sucks.
  Find a better way.
* File I/O (part of the os module?)
* External modules (e.g. "use foo;" should import foo.chimp at compile time).
* Sockets
* Expose methods on more classes.
* Rubyish symbols?
* Annotations?
* Immutable values ...
* Module definitions.
* VM stacks are made roots on VM creation, but are not removed from the root
  set at destruction. Either figure out a way to let VMs mark themselves, or
  provide a means of clearing elements from the root set.
* Are we rooting all the core classes correctly?
* Arbitrary precision for ChimpInt.
* ChimpFloat?
* Hash & array literals containing only constants can be optimized at compile time.
* Unicode! (lol)
* Cache bound methods on first access, or at object construction time if
  that makes more sense. Er. Do we even need 'em at all?
  Implicit "self" on call?
* Generational GC one day. Erlang-style GC algorithm switching another.
* Modules, code objects, builtins, etc. (read-only global data) probably
  belongs somewhere other than the 'main' task heap. Alternatively, don't
  bother spinning up a modules hash for non-main tasks.
