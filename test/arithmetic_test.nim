use nimunit
use io

main argv {

  # Addition tests
  nimunit.test("Test Int + Int", fn { |t|
    t.equals(1 + 2, 3)
    t.equals(-1 + 2, 1)
    t.equals(-1 + 1, 0)
    t.equals(1 - 1 - 5, -5)
  })

  nimunit.test("Test Float + Float", fn { |t|
    t.equals(1.5 + 2.0, 3.5)
    t.equals(-1.25 + 2.75, 1.5)
    t.equals(-1.0 + 1.0, 0.0)
  })

  nimunit.test("Test Int + Float", fn { |t|
    t.equals(1 + 2.5, 3.5)
    t.equals(-1.0 + 2, 1.)
    t.equals(-1 + 1.5 + 3.0, 3.5)
  })

  # Subtraction tests
  nimunit.test("Test Int - Int", fn { |t|
    t.equals(1 - 2, -1)
    t.equals(-1 - 2, -3)
    t.equals(1 - 1, 0)
  })

  nimunit.test("Test Float - Float", fn { |t|
    t.equals(1.5 - 2.0, -0.5)
    t.equals(-1.0 - 2.75, -3.75)
    t.equals(1.0 - 1.0, 0.0)
  })

  nimunit.test("Test Int - Float", fn { |t|
    t.equals(1 - 2.5, -1.5)
    t.equals(-1.0 - 2, -3.)
    t.equals( 1 - 1.25 - 1, -1.25)
  })

  # Multiplication tests
  nimunit.test("Test Int * Int", fn { |t|
    t.equals(2 * 2, 4)
    t.equals(-1 * 2, -2)
    t.equals(-5 * 10, -50)
  })

  nimunit.test("Test Float * Float", fn { |t|
    t.equals(1.0 * 2.0, 2.0)
    t.equals(-1.5 * 2.0, -3.0)
    t.equals(-1.25 * 2.0, -2.5)
  })

  nimunit.test("Test Int * Float", fn { |t|
    t.equals(1 * 2.0, 2.0)
    t.equals(-1.25 * 2, -2.5)
    t.equals(-10 * 1.1 * 0.5, -5.5)
  })

  # Division tests
  nimunit.test("Test Int / Int", fn { |t|
    t.equals(1 / 2, 0)
    t.equals(-1 / 2, 0)
    t.equals(1 / -2, 0)
    t.equals(4 / 2, 2)
    t.equals(-4 / -2, 2)
    t.equals(-4 / 2, -2)
    t.equals(4 / -2, -2)
  })

  nimunit.test("Test Float / Float", fn { |t|
    t.equals(1.0 / 2.0, 0.5)
    t.equals(-1.0 / 2.0, -0.5)
    t.equals(-1.0 / 1.0, -1.0)
    t.equals(-3.0 / 2.0, -1.5)
    t.equals(3.0 / 2.0, 1.5)
    t.equals(4.0 / 2.0, 2.0)
  })

  nimunit.test("Test Int / Float", fn { |t|
    t.equals(1 / 2.0, 0.5)
    t.equals(-1.0 / 2, -0.5)
    t.equals(-1 / 1.0, -1.0)
    t.equals(-3.0 / 2, -1.5)
    t.equals(3.0 / 2, 1.5)
    t.equals(4 / 2.0, 2.0)
    t.equals(4 / 2.0 / 2, 1.0)
  })
}
