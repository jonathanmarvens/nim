use nimunit

main argv {
  nimunit.test("grouped expressions 1", fn { |t|
    t.equals(9, (1 + 2) * 3)
  })
  nimunit.test("grouped expressions 2", fn { |t|
    t.equals(7, 1 + (2 * 3))
  })
  nimunit.test("grouped expressions 3", fn { |t|
    t.equals(47, 1 + (2 * (3 + (4 * 5))))
  })
  nimunit.test("binary boolean expr 1", fn { |t|
    t.equals(2, 1 and 2)
  })
  nimunit.test("binary boolean expr 2", fn { |t|
    t.equals(1, 2 and 1)
  })
  nimunit.test("binary boolean expr 3", fn { |t|
    t.equals(1, 1 or 2)
  })
  nimunit.test("binary boolean expr 4", fn { |t|
    t.equals(2, 2 or 1)
  })
  nimunit.test("binary boolean expr 5", fn { |t|
    t.equals(1, false or 1)
  })
  nimunit.test("binary boolean expr 6", fn { |t|
    t.equals(1, 1 or false)
  })
  nimunit.test("binary boolean expr 7", fn { |t|
    t.equals(nil, false or nil)
  })
  nimunit.test("chained binary arithmetic 1", fn { |t|
    t.equals(-5, 1 - 1 - 5)
  })
  nimunit.test("chained binary arithmetic 2", fn { |t|
    t.equals(1, 4 / 2 / 2)
  })
}
