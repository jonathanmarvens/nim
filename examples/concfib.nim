#
# This example is currently broken.
#

use io

fib n {
  if n > 1 {
    ret fib(n - 1) + fib(n - 2)
  }
  ret n
}

fib_task p, n {
  io.print(str("running fib(", n, ")"))
  p.send(fib(n))
}

main argv {
  var f3 = spawn fib_task(self(), 30)
  var f2 = spawn fib_task(self(), 20)
  var f1 = spawn fib_task(self(), 10)

  io.print(recv())
  io.print(recv())
  io.print(recv())

  f1.join()
  f2.join()
  f3.join()
}

