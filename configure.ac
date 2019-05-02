AC_INIT([telnet-site], [0.4], [https://gitlab.com/mb])
AM_INIT_AUTOMAKE([foreign])
AC_CONFIG_SRCDIR([configure.ac])
AC_CONFIG_HEADERS([config.h])
AC_PROG_CC
AC_PROG_CC_STDC
AC_CONFIG_FILES([Makefile src/Makefile])

AC_ARG_ENABLE([follow-links],
              AS_HELP_STRING([--disable-follow-links], [Disable following symbolic links when reading content files]))
AS_IF([test "x$enable_follow_links" != "xno"], [
       AC_DEFINE([FOLLOW_LINKS], [], [Follow symbolic links when traversing directories])
])

AC_ARG_ENABLE([unicode],
              AS_HELP_STRING([--enable-unicode], [Enable unicode support for content files (note: not filenames)]))
AS_IF([test "x$enable_unicode" = "xyes"],
      [
        PKG_CHECK_MODULES([ncurses], [ncursesw])
        AC_DEFINE([ENABLE_WCHAR], [], [Enable wide character (unicode) support])
      ],[
        PKG_CHECK_MODULES([ncurses], [ncurses])
      ])

AC_OUTPUT
