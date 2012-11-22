BUILT_SOURCES = \
	$(builddir)/ast.c \
	$(builddir)/include/chimp/ast.h \
	$(srcdir)/_parser.c \
	$(srcdir)/include/chimp/_parser.h \
	$(srcdir)/scanner.c

CLEANFILES = $(BUILT_SOURCES)

AM_YFLAGS = --defines=$(srcdir)/include/chimp/_parser.h

lib_LTLIBRARIES = libchimp.la

libchimp_la_SOURCES = \
	ast.c \
	_parser.y \
	scanner.l \
	gc.c \
	core.c \
	str.c \
	class.c \
	lwhash.c \
	object.c \
	array.c \
	task.c \
	method.c \
	frame.c \
	hash.c \
	int.c \
	code.c \
	compile.c \
	vm.c \
	module.c \
	modules/iomod.c \
	modules/assertmod.c \
	modules/osmod.c

include_HEADERS = \
	$(srcdir)/include/chimp.h \
	$(srcdir)/include/chimp/lwhash.h \
	$(srcdir)/include/chimp/task.h \
	$(srcdir)/include/chimp/class.h \
	$(srcdir)/include/chimp/str.h \
	$(srcdir)/include/chimp/frame.h \
	$(srcdir)/include/chimp/module.h \
	$(srcdir)/include/chimp/ast.h \
	$(srcdir)/include/chimp/_parser.h \
	$(srcdir)/include/chimp/method.h \
	$(srcdir)/include/chimp/code.h \
	$(srcdir)/include/chimp/int.h \
	$(srcdir)/include/chimp/vm.h \
	$(srcdir)/include/chimp/array.h \
	$(srcdir)/include/chimp/core.h \
	$(srcdir)/include/chimp/compile.h \
	$(srcdir)/include/chimp/object.h \
	$(srcdir)/include/chimp/hash.h \
	$(srcdir)/include/chimp/modules.h \
	$(srcdir)/include/chimp/any.h \
	$(srcdir)/include/chimp/gc.h

libchimp_la_CFLAGS = -Wall -I$(srcdir)/include -I$(builddir)/include @valgrind_CFLAGS@
libchimp_la_LIBADD =

scanner.c: $(srcdir)/include/chimp/_parser.h

include/chimp/ast.h: $(top_srcdir)/script/astgen
	mkdir -p include/chimp
	$(top_srcdir)/script/astgen include/chimp/ast.h ast.c

ast.c: $(top_srcdir)/script/astgen include/chimp/ast.h

