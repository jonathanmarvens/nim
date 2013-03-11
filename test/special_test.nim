use os
use nimunit

main argv {
  nimunit.test("__file__", fn { |t|
    t.equals(os.basename(__file__), "special_test.nim")
  })

  nimunit.test("__line__", fn { |t|
    t.equals(__line__, 10)
  })
}

