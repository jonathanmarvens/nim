ACLOCAL_AMFLAGS = --install -I m4

SUBDIRS = libchimp .

bin_PROGRAMS = chimp
chimp_SOURCES = main.c
chimp_CFLAGS = -Wall -I$(top_srcdir)/libchimp/include @valgrind_CFLAGS@
chimp_LDADD = $(top_builddir)/libchimp/libchimp.la
# TODO sniff out the appropriate pthread flags ...
chimp_LDFLAGS = -pthread

if HAVE_CHECK

TESTS = test/all-native

MOSTLYCLEANFILES = test/_test_main.c

common_check_cflags = -Wall -I$(top_srcdir)/test -I$(top_srcdir)/libchimp/include @valgrind_CFLAGS@ @check_CFLAGS@
common_check_ldadd = $(top_builddir)/libchimp/libchimp.la @check_LIBS@
common_check_ldflags = -pthread

check_PROGRAMS = test/all-native

test_all_native_SOURCES = $(builddir)/test/_test_main.c
test_all_native_CFLAGS = $(common_check_cflags)
test_all_native_LDADD = $(common_check_ldadd)
test_all_native_LDFLAGS = $(common_check_ldflags)

# XXX not portable. hard code me when things settle down.
test_sources = $(wildcard $(top_srcdir)/test/test_*.c)

$(builddir)/test/_test_main.c: \
	$(top_srcdir)/script/testgen \
	$(test_sources)
	script/testgen test/test_*.c >$@

endif

