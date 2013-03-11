use io
use nimunit

simple_task {
  recv().send("done")
}

multiple_messages {
  var origin = recv()
  range(0, 3).each(fn { |i|
    origin.send(i)
  })
  origin.send("done")
}

main argv {
  nimunit.test("simple task", fn { |t|
    var task = spawn simple_task()
    task.send(self())
    t.equals(recv(), "done")
  })

  nimunit.test("multiple messages", fn { |t|
    var task = spawn multiple_messages()
    task.send(self())

    var msgs = []
    while true {
      match recv() {
        "done" { break }
        i { msgs.push(i) }
      }
    }
    t.equals(msgs, [0, 1, 2])
  })
}

