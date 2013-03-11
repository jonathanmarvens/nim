use nimunit
use io

main argv {
  nimunit.test("int array comparison", fn { |t|
    t.equals([1, 2, 3], [1, 2, 3])
    t.not_equals([1, 2, 3], [3, 2, 1])
    t.not_equals([1, 2, 3], [1, 2, 3, 4])
  })

  nimunit.test("string array comparison", fn { |t|
    t.equals(["a", "b", "c"], ["a", "b", "c"])
    t.not_equals(["a", "b", "c"], ["c", "b", "a"])
    t.not_equals(["a", "b", "c"], ["a", "b", "c", "d"])
  })

  nimunit.test("mismatched comparison", fn { |t|
    t.not_equals([1], ["1"])
  })

  nimunit.test("contains", fn { |t|
    t.equals(true,  [1, 2, 3].contains(2))
    t.equals(false, [1, 2, 3].contains(5))
  })

  nimunit.test("map", fn { |t|
    var result = [1, 2, 3].map(str)
    t.equals(["1", "2", "3"], result)
  })

  nimunit.test("each", fn { |t|
    var sum = 0
    [1, 2, 3].each(fn { |i| sum = sum + i })
    t.equals(6, sum)
  })

  nimunit.test("filter", fn { |t|
    var result = [1, 2, 3].filter(fn { |i| ret i >= 2 })
    t.equals([2, 3], result)
  })

  nimunit.test("join", fn { |t|
    var result = ["1", "2", "3"].join(",")
    t.equals("1,2,3", result)
  })

  nimunit.test("concatenation with `+`", fn { |t|
    var result = [1, 2, 3] + [4, 5, 6]
    t.equals(result, [1, 2, 3, 4, 5, 6])
  })

  nimunit.test("remove element", fn { |t|
    var items = ["a", "b", "c"]
    t.equals(items.remove("b"), true)
    t.equals(items, ["a", "c"])
    t.equals(items.remove("c"), true)
    t.equals(items, ["a"])
    t.equals(items.remove(nil), false)
    t.equals(items.remove("a"), true)
    t.equals(items, [])
  })

  nimunit.test("remove element at index", fn { |t|
    var items = [1, 2, 3]
    t.equals(items.remove_at(0), 1)
    t.equals(items.remove_at(1), 3)
    t.equals(items.remove_at(0), 2)
    t.equals(items.size(), 0)
  })

  nimunit.test("slice", fn { |t|
    var items = [1, 2, 3, 4]
    t.equals([2, 3], items.slice(1, 3))
  })

  nimunit.test("insert", fn { |t|
    var items = [0, 1, 2]
    items.insert(1, 3)
    t.equals([0, 3, 1, 2], items)
    items.insert(3, 4)
    t.equals([0, 3, 1, 4, 2], items)
    items.insert(5, 5)
    t.equals([0, 3, 1, 4, 2, 5], items)
    items.insert(0, -1)
    t.equals([-1, 0, 3, 1, 4, 2, 5], items)
  })

  nimunit.test("shift", fn { |t|
    var items = [0, 2, 4]
    t.equals(items.shift(), 0)
    t.equals(items.shift(), 2)
    t.equals(items.shift(), 4)
  })

  nimunit.test("any integers", fn { |t|
    t.equals(true, [0, 1, 0].any(bool))
    t.equals(false, [0, 0, 0].any(bool))
  })

  nimunit.test("any strings", fn { |t|
    t.equals(true, ["Hello", "World", "!"].any(bool))
    t.equals(false, ["", "", ""].any(bool))
  })
}
