use io

main argv {
  var task = spawn {
    while true {
      match recv() {
        ["say", msg] {
          io.print(msg)
        }
        "die" {
          ret
        }
        _ {
          io.print("ignoring unknown message")
        }
      }
    }
  }

  task.send(["say", "Hello from another thread"])
  task.send("asdkjaksd")
  task.send("die")
  task.join()
}

