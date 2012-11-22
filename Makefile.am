ACLOCAL_AMFLAGS = --install -I m4

SUBDIRS = libchimp .

bin_PROGRAMS = chimp
chimp_SOURCES = main.c
chimp_CFLAGS = -Wall -I$(srcdir)/libchimp/include @valgrind_CFLAGS@
chimp_LDADD = libchimp/libchimp.la
# TODO sniff out the appropriate pthread flags ...
chimp_LDFLAGS = -pthread

BUILT_SOURCES = test/_test_main.c

MOSTLYCLEANFILES = test/_test_main.c
EXTRA_DIST = \
	script/astgen \
	script/testgen \
	script/gdb \
	script/valgrind \
	test/_test_main.c \
	test/test_class.c \
	test/test_str.c \
	test/test_core.c

if HAVE_CHECK

test/_test_main.c: $(srcdir)/script/testgen $(test_sources) \
	$(srcdir)/script/testgen \
	$(test_sources)
	script/testgen test/test_*.c >$@

TESTS = test/all-native

common_check_cflags = -Wall -I$(srcdir)/test -I$(srcdir)/libchimp/include @valgrind_CFLAGS@ @check_CFLAGS@
common_check_ldadd = $(builddir)/libchimp/libchimp.la @check_LIBS@
common_check_ldflags = -pthread

check_PROGRAMS = test/all-native

test_all_native_SOURCES = test/_test_main.c
test_all_native_CFLAGS = $(common_check_cflags)
test_all_native_LDADD = $(common_check_ldadd)
test_all_native_LDFLAGS = $(common_check_ldflags)

# XXX not portable. hard code me when things settle down.
test_sources = $(wildcard $(srcdir)/test/test_*.c)

endif

