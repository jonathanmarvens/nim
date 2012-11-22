ACLOCAL_AMFLAGS = --install -I m4

SUBDIRS = libchimp .

bin_PROGRAMS = chimp
chimp_SOURCES = main.c
chimp_CFLAGS = -Wall -I$(srcdir)/libchimp/include -I$(builddir)/libchimp/include @valgrind_CFLAGS@
chimp_LDADD = $(builddir)/libchimp/libchimp.la
# TODO sniff out the appropriate pthread flags ...
chimp_LDFLAGS = -pthread

MOSTLYCLEANFILES = $(builddir)/test/_test_main.c
EXTRA_DIST = \
	$(srcdir)/script/astgen \
	$(srcdir)/script/testgen \
	$(srcdir)/script/gdb \
	$(srcdir)/script/valgrind \
	$(srcdir)/test/test_class.c \
	$(srcdir)/test/test_str.c \
	$(srcdir)/test/test_core.c

$(builddir)/test/_test_main.c: $(srcdir)/script/testgen $(test_sources) \
	$(srcdir)/script/testgen \
	$(test_sources)
	script/testgen test/test_*.c >$@

if HAVE_CHECK

TESTS = test/all-native

common_check_cflags = -Wall -I$(builddir)/test -I$(srcdir)/libchimp/include -I$(builddir)/libchimp/include @valgrind_CFLAGS@ @check_CFLAGS@
common_check_ldadd = $(builddir)/libchimp/libchimp.la @check_LIBS@
common_check_ldflags = -pthread

check_PROGRAMS = test/all-native

test_all_native_SOURCES = $(builddir)/test/_test_main.c
test_all_native_CFLAGS = $(common_check_cflags)
test_all_native_LDADD = $(common_check_ldadd)
test_all_native_LDFLAGS = $(common_check_ldflags)

# XXX not portable. hard code me when things settle down.
test_sources = $(wildcard $(srcdir)/test/test_*.c)

endif

