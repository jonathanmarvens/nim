use nimunit

main argv {
  nimunit.test("hash with string keys", fn { |t|
    var h = {"foo": 1, "bar": 2}
    t.equals(h["foo"], 1)
    t.equals(h["bar"], 2)
  })
  nimunit.test("hash to string", fn { |t|
    var h = {}
    t.equals(str(h), "{}")
  })
  nimunit.test("items", fn { |t|
    var h = {"foo": "bar", "bar": "baz"}
    # XXX relying on ordering here is dumb.
    t.equals(h.items(), [["bar", "baz"], ["foo", "bar"]])
  })
}

