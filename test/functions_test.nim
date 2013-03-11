use nimunit

incr n {
  n + 1
}
main argv {
  nimunit.test("basic function call", fn { |t|
    t.equals(1, incr(0))
  })

  nimunit.test("function as variable", fn { |t|
    var double = fn { |n| n * 2 }
    t.equals(2, double(1))
  })

  nimunit.test("closure", fn { |t|
    var i = 0
    fn { i = i + 1 }()
    t.equals(1, i)
  })
}
