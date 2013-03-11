#!/usr/local/bin/nim
#
# Dirty sandbox.
#

use io;
use os;

class animal {
}

class dog : animal {
  init {
    io.print("dog initialized");
  }

  bark {
    io.print("woof!");
  }
}

main argv {
  var x = "hello";
  var y = ["foo"];
  var z = x;

  io.print(hash(), z, y, str(z), {"foo": "bar", "bar": "baz"}, true, 9223372036854775807, 3.14);

  var some_func = fn { |x| io.print(str("a function! ", x)); };
  some_func("testing");

  var num_array = [1, 2, 3];
  var str_array = num_array.map(str);
  if str_array.contains("1") {
    io.print("str_array contains '1'!");
  }
  io.print(num_array);
  io.print(str_array);
  num_array.each(io.print);

  if ["a", "b", "c"].filter(fn { |x| ret x != "b"; }).contains("b") {
    io.print("error!!! array should not contain 'b'");
    ret;
  }

  var some_bool = false;
  if some_bool {
    io.print (x);
  }
  else {
    io.print ("omgbye");
  }

  var i = 0;
  while i < 4 {
    io.print (str("while ", i));
    i = i + 1;
    if i == 3 {
      io.print("i == 3: breaking!");
      break;
    }
  }

  while not true {
    io.print ("this should never run");
  }

  io.print ("Enter your name:");
  var name = io.readline();
  if name == "dude" {
      io.print ("Dude!");
  }
  else {
      io.print (str("Hey there, ", name));
  }

  var home = os.getenv ("HOME");
  if home {
    io.print (home);
  }

  io.print (true and "test");

  var m = module("main");
  io.print (str(m));

  var two = 2;
  io.print (str(1 + two * 3));
  io.print (str(1 + 2 and 2 + 3));

  var match_me = [
    "first item",
    {"key": "value", "foo": "bar"},
    10,
    []
  ];
  match match_me {
    [item1, {"key": "value", "foo": bar}, 10, _] {
      io.print(str(item1, ", ", bar));
    }
  }

  var arr = [1, "two"];
  io.print(arr[0]);
  io.print(arr[1]);

  var h = {"foo": "bar"};
  io.print(h["foo"]);

  var lookup = {
    "a": fn { |x| io.print(x); },
    "b": fn { |x| io.print(x * 2); }
  };
  lookup["a"](5);
  lookup["b"](5);

  io.print(class(lookup));
  io.print(class(lookup).super);

  var d = dog();
  d.bark();
  io.print(str(d));

  io.print(arr);
  fn { arr.push("added from closure"); }();
  io.print(arr);

  io.print([1, 2, 3].join(", "));
}

