AC_INIT([chimp],[0.0.1],[github@tomlee.co])
AC_CANONICAL_SYSTEM
AM_INIT_AUTOMAKE([1.9 foreign])

AC_CONFIG_MACRO_DIR([m4])

PKG_CHECK_MODULES([valgrind],[valgrind >= 3.0.0],[HAVE_VALGRIND="yes"],[HAVE_VALGRIND="no"])
AC_SUBST(valgrind_CFLAGS)
AS_IF([test "x$HAVE_VALGRIND" = "xyes"],[AC_DEFINE([HAVE_VALGRIND])])

AC_PROG_LIBTOOL
AC_PROG_CC

AC_CONFIG_FILES([Makefile libchimp/Makefile])

AC_OUTPUT

