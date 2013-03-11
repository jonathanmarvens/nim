use nimunit

main argv {
  nimunit.test("range single arg", fn { |t|
    t.equals(range(5), [0, 1, 2, 3, 4])
  })

  nimunit.test("range two arg", fn { |t|
    t.equals(range(5, 10), [5, 6, 7, 8, 9])
  })

  nimunit.test("range three arg", fn { |t|
    t.equals(range(0, 10, 2), [0, 2, 4, 6, 8])
  })

  nimunit.test("range single case", fn { |t|
    t.equals(range(1), [0])
    t.equals(range(1, 5, 6), [1])
  })

  nimunit.test("range empty cases", fn { |t|
    t.equals(range(-1), [])
    t.equals(range(1, 1), [])
    t.equals(range(5, 0), [])
    t.equals(range(-11, -12), [])
  })

  nimunit.test("range negative cases", fn { |t|
    t.equals(range(-8, -4), [-8, -7, -6, -5])
    t.equals(range(5, 0, -1), [5, 4, 3, 2, 1])
    t.equals(range(4, -4, -2), [4, 2, 0, -2])
  })
}
