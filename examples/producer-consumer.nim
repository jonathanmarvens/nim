use io

producer_task {
  var tests = [1, 2, 3]
  var i = 0

  while true {
    io.print("[producer] recv()")
    match recv() {
      [consumer, "ready"] {
        io.print("[producer] send()")
        if i < tests.size() {
          consumer.send(["test", tests[i]])
          i = i + 1
        } else {
          consumer.send("done")
        }
      }
      "done" {
        break
      }
      other {
        io.print(str("[producer] unknown message: ", other))
      }
    }
  }
}

consumer_task {
  var producer = recv()
  while true {
    io.print(str("[", self(), "] send('ready')"))
    producer.send([self(), "ready"])
    io.print(str("[", self(), "] recv()"))
    match recv() {
      ["test", test] {
        io.print(str("test ", test, " on consumer ", self()))
      }
      "done" {
        break
      }
      other {
        io.print(str("[", self(), "] unknown message: ", other))
      }
    }
  }
  io.print("consumer is done")
}

main argv {
  var producer = spawn producer_task()

  var consumers = []
  var i = 0
  range(0, 2).each(fn { |i|
    consumers.push(spawn consumer_task())
  })
  consumers.each(fn { |c| c.send(producer) })
  consumers.each(fn { |c| c.join() })
  io.print("consumers are done")
  producer.send("done")
  producer.join()
  io.print("producer is done")
}

