ACLOCAL_AMFLAGS = --install -I m4

SUBDIRS = libchimp .

bin_PROGRAMS = chimp
chimp_SOURCES = main.c
chimp_CFLAGS = -Wall -I$(top_srcdir)/libchimp/include @valgrind_CFLAGS@
chimp_LDADD = $(top_builddir)/libchimp/libchimp.la
# TODO sniff out the appropriate pthread flags ...
chimp_LDFLAGS = -pthread

if HAVE_CHECK

TESTS = test/core

MOSTLYCLEANFILES = test/_test_core.c

check_PROGRAMS = test/core
test_core_SOURCES = $(builddir)/test/_test_core.c
test_core_CFLAGS = -Wall -I$(top_srcdir)/test -I$(top_srcdir)/libchimp/include @valgrind_CFLAGS@ @check_CFLAGS@
test_core_LDADD = $(top_builddir)/libchimp/libchimp.la @check_LIBS@
test_core_LDFLAGS = -pthread

$(builddir)/test/_test_core.c: $(top_srcdir)/test/core.c
	script/testgen $< >$@

endif

