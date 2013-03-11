use os;
use io;

main argv {
  var hello = compile(os.dirname(__file__) + "/helloworld.nim");
  hello.main();
}
