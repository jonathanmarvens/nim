#
# This example is currently broken.
#

use io;

fib n {
  if n > 1 {
    ret fib(n - 1) + fib(n - 2);
  }
  ret n;
}

main argv {
  io.print("Running fib(10), fib(20), fib(30) concurrently ...");

  var f3 = spawn { recv().send(fib(30)); };
  var f2 = spawn { recv().send(fib(20)); };
  var f1 = spawn { recv().send(fib(10)); };

  f1.send(self());
  f2.send(self());
  f3.send(self());

  io.print(recv());
  io.print(recv());
  io.print(recv());

  f1.join();
  f2.join();
  f3.join();
}

