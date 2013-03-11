use io;

main argv {
  var arg1 = range(10);
  io.print(arg1);
  var arg2 = range(0, 20);
  io.print(arg2);
  var arg3 = range(0, 20, 2);
  io.print(arg3);
  var single_case0 = range(1, 5, 6);
  io.print(single_case0);

  var empty_case0 = range(1, 1);
  io.print(empty_case0);
  var empty_case1 = range(1, 0);
  io.print(empty_case1);
  var empty_case2 = range(-11, -12);
  io.print(empty_case2);

  var neg0 = range(5, 0, -1);
  io.print(neg0);
  var neg1 = range(4, -4, -2);
  io.print(neg1);
  var neg2 = range(-8, -4);
  io.print(neg2);

  var fail_case0 = range(0, 1, 0);
  var fail_case1 = range(0, 1, 0, 0);

}
