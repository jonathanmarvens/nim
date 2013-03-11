use io

pong {
  io.print(str("ponger starts"))
  var pinger = recv()
  io.print(str("ponger got pinger: ", pinger))
  while true {
    match recv() {
      "ping" {
        io.print("ping")
        io.print("ponger sending")
        pinger.send("pong")
        io.print("ponger waiting")
      }
      "finished" {
        ret
      }
    }
  }
}

ping {
  io.print(str("pinger starts"))
  var ponger = recv()
  io.print(str("pinger got ponger: ", ponger))
  var i = 0
  while i < 3 {
    io.print("pinger sending")
    ponger.send("ping")
    io.print("pinger waiting")
    io.print(recv())
    i = i + 1
  }
  ponger.send("finished")
}

main argv {
  var ponger = spawn pong()
  var pinger = spawn ping()
  ponger.send(pinger)
  pinger.send(ponger)
  ponger.join()
  pinger.join()
}

