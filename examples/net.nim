use io;
use net;

main argv {
  var srv = net.socket (net.AF_INET, net.SOCK_STREAM, 0);
  srv.setsockopt (net.SOL_SOCKET, net.SO_REUSEADDR, 1);
  srv.bind(5123);
  srv.listen();
  while true {
    var c = srv.accept();
    var task = spawn {
      var sck = net.socket(recv());
      io.print(sck.recv(8192));
      sck.send("hello there\n");
      sck.shutdown (net.SHUT_WR);
      while true {
        if sck.recv(8192) == "" {
          break;
        }
      }
      sck.close();
    };
    task.send(c.fd);
  }
  srv.close();
}

