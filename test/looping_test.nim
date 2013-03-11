use nimunit

main argv {
  nimunit.test("while", fn { |t|
    var i = 0
    while i < 5 {
      i = i + 1
    }
    t.equals(5, i)
  })

  nimunit.test("break it down", fn { |t|
    var i = 0
    while true {
      i = i + 1
      if i == 10 {
        break
      }
    }
    t.equals(10, i)
  })
}
