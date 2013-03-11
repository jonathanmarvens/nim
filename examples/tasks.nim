#
# This example is currently broken.
#

use io;
use os;

main argv {
  var task = spawn { |me|
    #
    # Let the user know we're not just twiddling our thumbs.
    #
    io.print("hello, from a background task");

    #
    # Testing recv()
    #
    var m2 = me.recv();
    io.print(m2);
    me.send("ohai");

    #
    # let's see if orphans get detached properly.
    # Inner task should be abandoned after f.join() returns.
    #
    var f = spawn { |me2|
      spawn { |me3|
        os.sleep(3);
        io.print("orphan is done");
      };
    };
    f.join();
  };

  var task2 = spawn { |me|
    os.sleep(5);
  };

  #
  # Testing send()
  # TODO: do away with user-land msg object
  #
  task.send(["msg", 1]);
  task.recv();
  task2.join();
  task.join();
}

