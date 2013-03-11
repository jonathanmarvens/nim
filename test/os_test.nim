use os
use nimunit

main argv {
  var path = "/path/to/foo.txt"

  nimunit.test("os.basename", fn { |t|
    t.equals("foo.txt", os.basename(path))
  })

  nimunit.test("os.dirname", fn { |t|
    t.equals("/path/to", os.dirname(path))
  })

  nimunit.test("os.dirname with relative path 1", fn { |t|
    t.equals(".", os.dirname("foo.txt"))
  })

  nimunit.test("os.dirname with relative path 2", fn { |t|
    t.equals("..", os.dirname("../foo.txt"))
  })

  nimunit.test("os.getenv", fn { |t|
    t.is_not_nil(os.getenv("HOME"))
  })

  nimunit.test("os.glob", fn { |t|
    var basedir = str(os.dirname(__file__), "/../")
    #
    # glob results are sorted.
    #
    var expected = [
      str(basedir, "main.c")
    ]
    t.equals(os.glob(str(basedir, "*.c")), expected)
  })
}

