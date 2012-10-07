GENERATED_SOURCES = ast.c ast.h
MOSTLYCLEANFILES = $(GENERATED_SOURCES)

lib_LTLIBRARIES = libchimp.la
libchimp_la_SOURCES = gc.c core.c str.c class.c lwhash.c object.c array.c task.c method.c stackframe.c hash.c int.c ast.c
libchimp_la_CFLAGS = -Wall -I$(srcdir)/include @valgrind_CFLAGS@
libchimp_la_LIBADD =

ast.h: ast.c

ast.c: $(top_srcdir)/script/astgen
	$(top_srcdir)/script/astgen $(srcdir)/include/chimp/ast.h $(srcdir)/ast.c

