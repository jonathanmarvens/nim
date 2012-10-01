ACLOCAL_AMFLAGS = --install -I m4

SUBDIRS = libchimp .

bin_PROGRAMS = chimp
chimp_SOURCES = main.c
chimp_CFLAGS = -Wall -I$(top_srcdir)/libchimp/include
chimp_LDADD = $(top_builddir)/libchimp/libchimp.la
# TODO sniff out the appropriate pthread flags ...
chimp_LDFLAGS = -pthread

