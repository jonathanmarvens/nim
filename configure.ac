AC_PREREQ(2.69)
AC_INIT([chimp],[0.0.1],[github@tomlee.co],[chimp],[http://github.com/thomaslee/chimp])
AC_CANONICAL_SYSTEM
AM_INIT_AUTOMAKE([1.9 foreign])

AC_CONFIG_MACRO_DIR([m4])

MIN_VALGRIND_VERSION=3.0.0
MIN_CHECK_VERSION=0.9.0

PKG_PROG_PKG_CONFIG([0.9.0])
AS_IF([test "x$PKG_CONFIG" = "x"],[AC_MSG_ERROR([pkg-config not installed])])

PKG_CHECK_MODULES([valgrind],[valgrind >= $MIN_VALGRIND_VERSION],[HAVE_VALGRIND="yes"],[HAVE_VALGRIND="no"])
AC_SUBST(valgrind_CFLAGS)
AS_IF([test x$HAVE_VALGRIND = xyes],[AC_DEFINE([HAVE_VALGRIND])])

PKG_CHECK_MODULES([check],[check >= $MIN_CHECK_VERSION],[HAVE_CHECK="yes"],[HAVE_CHECK="no"])
AS_IF([test x$HAVE_CHECK = xno],[AC_MSG_WARN([check not detected: no tests available])])
AC_SUBST(check_CFLAGS)
AC_SUBST(check_LIBS)
AM_CONDITIONAL([HAVE_CHECK],[test x$HAVE_CHECK = xyes])

AC_PROG_YACC
AC_PROG_LEX
AC_PROG_LIBTOOL
AC_PROG_CC

AC_CONFIG_FILES([Makefile libchimp/Makefile])

AC_OUTPUT

