AC_INIT([telnet-site], [0.4], [https://gitlab.com/mb])
AM_INIT_AUTOMAKE([foreign])
AC_CONFIG_SRCDIR([configure.ac])
AC_CONFIG_HEADERS([config.h])
AC_PROG_CC
AC_PROG_CC_STDC
AC_CONFIG_FILES([Makefile src/Makefile])
AC_ARG_ENABLE([follow-links],
              AS_HELP_STRING([--disable-follow-links], [Disable following symbolic links when reading site sections]))
AS_IF([test "x$enable_follow_links" != "xno"], [
       AC_DEFINE([FOLLOW_LINKS], [], [Do not follow links when traversing directories])
])
PKG_CHECK_MODULES([ncurses], [ncurses])
AC_OUTPUT
