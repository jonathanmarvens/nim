GENERATED_FILES = \
	$(srcdir)/ast.c \
	$(srcdir)/include/chimp/ast.h \
	$(srcdir)/_parser.c \
	$(srcdir)/include/chimp/_parser.h \
	$(srcdir)/scanner.c

MOSTLYCLEANFILES = $(GENERATED_FILES)

AM_YFLAGS = --defines=$(top_srcdir)/include/chimp/_parser.h

lib_LTLIBRARIES = libchimp.la
libchimp_la_SOURCES = \
	_parser.y \
	scanner.l \
	ast.c \
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

libchimp_la_CFLAGS = -Wall -I$(srcdir)/include @valgrind_CFLAGS@
libchimp_la_LIBADD =

ast.h: ast.c

ast.c: $(top_srcdir)/script/astgen
	$(top_srcdir)/script/astgen $(srcdir)/include/chimp/ast.h $(srcdir)/ast.c

