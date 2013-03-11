use io
use gc

fib n {
  if n > 1 {
    ret fib(n - 1) + fib(n - 2)
  }
  ret n
}

gc_stats {
  io.print(str("GC collections: ", gc.get_collection_count()))
  io.print(str("Live slots: ", gc.get_live_count()))
  io.print(str("Free slots: ", gc.get_free_count()))
}

main argv {
  io.print(fib(20))

  io.print("")
  gc_stats()

  io.print("")
  io.print("Running the garbage collector ...")
  io.print("")

  gc.collect()

  gc_stats()
}

