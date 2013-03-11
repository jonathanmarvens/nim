use nimunit

main argv {
  nimunit.test("string escapes", fn { |t|
    t.equals("abcdef\n".size(), 7)
  })

  nimunit.test("newline in string", fn { |t|
    t.equals("abcdef
foo", "abcdef\nfoo")
  })

  nimunit.test("concatenation with `+`", fn { |t|
    t.equals("foo" + "bar", "foobar")
  })

  nimunit.test("trim", fn { |t|
    t.equals("foo ".trim(), "foo")
    t.equals("  bar".trim(), "bar")
    t.equals("  baz  ".trim(), "baz")
    t.equals("/".trim(), "/")
  })

  nimunit.test("index", fn { |t|
    var s = "cat"
    t.equals(s.index("a"), 1)
    t.equals(s.index("c"), 0)
    t.equals(s.index("t"), 2)
    t.equals(s.index("b"), -1)
  })

  nimunit.test("substr", fn { |t|
    var s = "bert banana"
    t.equals(s.substr(0, 4), "bert")
    t.equals(s.substr(-6), "banana")
    t.equals(s.substr(0, -1), "bert banan")
    t.equals(s.substr(s.index(" ") + 1), "banana")
  })

  nimunit.test("split 1", fn { |t|
    var arr = "1, 2, 3"
    t.equals(arr.split(", "), ["1", "2", "3"])
  })

  nimunit.test("split 2", fn { |t|
    var reqline = "GET / HTTP/1.0"
    var parts = reqline.split(" ")
    t.equals(parts, ["GET", "/", "HTTP/1.0"])
  })

  nimunit.test("upper", fn { |t|
    var s = "abcdef123"
    t.equals(s.upper(), "ABCDEF123")
  })

  nimunit.test("lower", fn { |t|
    var s = "ABCDEF123"
    t.equals(s.lower(), "abcdef123")
  })
}

