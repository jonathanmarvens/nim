use nimunit


main argv {

	nimunit.test("Bool construct integer", fn { |t|
		t.equals(true, bool(1))
		t.equals(false, bool(0))
		t.equals(false, bool(-1))
	})

	nimunit.test("Bool construct string", fn { |t|
		t.equals(true, bool("Hello, World"))
		t.equals(false, bool(""))
	})

	nimunit.test("Bool construct list", fn { |t|
		t.equals(true, bool([1, 2, 4]))
		t.equals(false, bool([]))
	})

	nimunit.test("Bool construct float", fn { |t|
		t.equals(true, bool(1.0))
		t.equals(false, bool(0.0))
	})

	nimunit.test("Bool construct hash", fn { |t|
		t.equals(true, bool({"key": "value"}))
		t.equals(false, bool({}))
	})

}
