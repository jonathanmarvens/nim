use os;
use io;
use assert;

main argv {
  var task = spawn {
    var parent = recv();
    parent.send("Message home to parent!");
  };

  task.send(self());
  io.print(["Parent received:", recv()]);
}
