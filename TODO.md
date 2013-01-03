# General
* Fix broken examples.
* Forward declarations.
* Use CHIMP\_SUPER(self) to invoke super class slots for e.g. init, dtor, etc.
* Allow (de)serialization of custom types to allow these objects to be sent
  as messages between tasks where safe (e.g. net.socket).
* Interesting note: because closures may capture parts of their execution
  environment & thus dependendent upon the state of the heap, it's
  rarely kosher to send closures to other tasks. (n.b. we can't send any
  kind of method between tasks atm -- perhaps best to keep it that way?)
* Constants/read-only vars: accessible between tasks without message passing.
* Verify scoping rules using the symtable.
  * Identifiers should not be used/referenced before declared ('var' keyword)
  * Ensure variables in the outer scope are not accessible inside spawn blocks
  * If we can determine a variable has been used without being initialized,
    complain.
  * Scope rules for vars bound during pattern matching? Scoped to inner block?
    Scoped to the function like other declared variables?
  * How should we deal with duplicate declarations? Thinking errors ...
* Implementing OOP (specifically: user classes) properly:
  * (meta: drop user classes for now?)
  * PUSHTHIS opcode.
  * setattr support (useful even if we ditch user classes)
  * chaining methods/constructors (e.g. "super" or "inner" from chimp,
macro from C?) -- we *should* be doing this already.
  * "super" keyword? "inner" keyword?
  * chain existing methods/constructors.
* setitem opcode for hash/array element mutation syntax (arr[1] = 10)
* Line number info for code objects.
* slice syntax for arrays (arr[1:2])
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
* Bitwise operators.
* Modulo operator.
* In-place operators for integers: +=, -=, /=, \*=, etc.
* Unary operators ++, --
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

# Windows
* Bison & Flex are a pain with MSVC under windows. Options: MinGW? Lemon?
* Some CMake quirks under Windows, too.

