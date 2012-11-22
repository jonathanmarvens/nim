BUILT_SOURCES = \
	ast.c \
	_parser.c \
	scanner.c

EXTRA_DIST = \
	include/chimp/_parser.h \
	include/chimp/ast.h

CLEANFILES = $(BUILT_SOURCES)

AM_YFLAGS = --defines=$(abs_top_srcdir)/libchimp/include/chimp/_parser.h

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
	include/chimp.h \
	include/chimp/lwhash.h \
	include/chimp/task.h \
	include/chimp/class.h \
	include/chimp/str.h \
	include/chimp/frame.h \
	include/chimp/module.h \
	include/chimp/ast.h \
	include/chimp/_parser.h \
	include/chimp/method.h \
	include/chimp/code.h \
	include/chimp/int.h \
	include/chimp/vm.h \
	include/chimp/array.h \
	include/chimp/core.h \
	include/chimp/compile.h \
	include/chimp/object.h \
	include/chimp/hash.h \
	include/chimp/modules.h \
	include/chimp/any.h \
	include/chimp/gc.h

libchimp_la_CFLAGS = -Wall -I$(srcdir)/include @valgrind_CFLAGS@
libchimp_la_LIBADD =

include/chimp/_parser.h: _parser.c

scanner.c: include/chimp/_parser.h

include/chimp/ast.h: $(top_srcdir)/script/astgen
	mkdir -p include/chimp
	$(top_srcdir)/script/astgen include/chimp/ast.h ast.c

ast.c: $(top_srcdir)/script/astgen include/chimp/ast.h

