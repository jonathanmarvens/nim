use nimunit

main argv {
  nimunit.test("if positive", fn { |t|
    var executed = false
    if true {
      executed = true
    }

    t.equals(true, executed)
  })

  nimunit.test("if negative", fn { |t|
    if false {
      t.fail("Executed false branch!")
    }
  })

  nimunit.test("else", fn { |t|
    var executed = false
    if false {
      t.fail("Executed false branch!")
    } else {
      executed = true
    }

    t.equals(true, executed)
  })
}
