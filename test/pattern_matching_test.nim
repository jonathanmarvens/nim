use nimunit

main argv {
  nimunit.test("match string literal", fn { |t|
    var x = "foo"
    var matched = false
    match x {
      "foo" { matched = true }
    }
    t.equals(matched, true)
  })

  nimunit.test("match integer", fn { |t|
    var x = 10
    var matched = false
    match x {
      10 { matched = true }
    }
    t.equals(matched, true)
  })

  nimunit.test("match empty array", fn { |t|
    var x = []
    var matched = false
    match x {
      [] { matched = true }
    }
    t.equals(matched, true)
  })

  nimunit.test("match wildcard", fn { |t|
    var x = 10
    var matched = false
    match x {
      _ { matched = true }
    }
    t.equals(matched, true)
  })

  nimunit.test("match failure", fn { |t|
    var x = 10
    var matched = false
    match x {
      "foo" { matched = true }
    }
    t.equals(matched, false)
  })

  nimunit.test("match with multiple patterns", fn { |t|
    var x = 10
    var matched = 0
    match x {
      "foo" { matched = 1 }
      10    { matched = 2 }
    }
    t.equals(matched, 2)
  })

  nimunit.test("match with variable binding", fn { |t|
    var x = 1
    var matched = 0
    match x {
      y { matched = y }
    }
    t.equals(matched, x)
  })

  nimunit.test("match with array unpacking", fn { |t|
    var x = [1, 2, 3, "foo"]
    var matched = []
    match x {
      [a, b, c, d] { matched = [a, b, c, d] }
    }
    t.equals(matched, x)
  })

  nimunit.test("match with hash unpacking", fn { |t|
    var x = {"foo": "bar", "baz": 10}
    var matched = []
    match x {
      {"foo": a, "baz": b} { matched = [a, b] }
    }
    t.equals(matched, ["bar", 10])
  })

  nimunit.test("match with array unpacking and wildcards", fn { |t|
    var x = [1, 2, "foo", "bar"]
    var matched = []
    match x {
      [a, b, _, c] { matched = [a, b, c] }
    }
    t.equals(matched, [1, 2, "bar"])
  })

  nimunit.test("match with nil", fn { |t|
    var x = nil
    var matched = false
    match x {
      nil { matched = true }
    }
    t.equals(matched, true)
  })

  nimunit.test("match with booleans", fn { |t|
    var x = false
    var matched = false
    match x {
      false { matched = true }
    }
    t.equals(matched, true)
  })

  nimunit.test("match str against array", fn { |t|
    var x = "foo"
    var y = 0
    match x {
      ["foo", bar] { y = 1 }
      "foo" { y = 2 }
    }
    t.equals(y, 2)
  })

  nimunit.test("match array against str", fn { |t|
    var x = ["foo", "bar"]
    var y = 0
    match x {
      ["foo", bar] { y = 1 }
      "foo" { y = 2 }
    }
    t.equals(y, 1)
  })

  nimunit.test("match array against multiple", fn { |t|
    var x = ["foo", "bar"]
    var y = 0
    match x {
      ["baz", z] { y = 1 }
      ["foo", z] { y = 2 }
      m { y = 3 }
    }
    t.equals(y, 2)
  })
}

