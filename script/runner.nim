#!/usr/local/bin/nim

use io
use os

main argv {
  var testdir = argv[1]
  if testdir== nil {
    testdir = os.dirname(__file__) + "/../test"
  }

  io.print("Starting Nim Runner for " + testdir)
  var n = 0
  os.glob(testdir + "/*.nim").each(fn { |f|
    var mod = compile(f)
    mod.main([])
  })
}
