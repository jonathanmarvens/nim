* Add the `self` keyword.
* getitem syntax.
* Verify scoping rules using the symtable.
  * Identifiers have function, module or 'builtin' scope.
  * Identifiers should not be used/referenced before declared ('var' keyword)
  * Ensure variables in the outer scope are not accessible inside spawn blocks
  * If we can determine a variable has been used without being initialized,
    complain.
  * Scope rules for vars bound during pattern matching? Scoped to inner block?
    Scoped to the function like other declared variables?
  * How should we deal with duplicate declarations? Thinking errors ...
* Error handling. Differentiate between runtime/compile errors & chimp bugs.
* Unit testing API.
* More tests.
* Make it possible to use other chimp source modules.
  (e.g. 'use foo' compiles & loads 'foo.chimp')
* 'use foo.bar;'
* Less semicolons.
* Revisit the bytecode format.
  - Variable length instructions? 8-bit args are limited (also, 24-bit jumps :()
* The task API is sucky and inconsistent with the rest of the codebase
  (sometimes ChimpTaskInternal, sometimes ChimpTask)
* Syntax for structured data. (class definitions?)
* Bitwise operators.
* Modulo operator.
* In-place operators for integers: +=, -=, /=, \*=, etc.
* Unary operators ++, --
* chimp_hash_get returning NULL and chimp\_nil completely sucks.
  Find a better way.
* Array slices.
* File I/O (part of the os module?)
* External modules (e.g. "use foo;" should import foo.chimp at compile time).
* Sockets
* Expose methods on more classes.
* Rubyish symbols?
* Annotations?
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
