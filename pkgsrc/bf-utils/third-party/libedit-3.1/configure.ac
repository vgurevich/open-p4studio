#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.
#
# Compile with debug symbols:
#    CFLAGS="-ggdb -pedandic -O0" ./configure
#    CFLAGS="-ggdb -Wall -Wextra -pedantic -O0" ./configure
#
# Use libtool (glibtool on OSX) to debug:
#
#    libtool --mode=execute gdb examples/tc1
# 
# Run valgrind like this, but note
# http://invisible-island.net/ncurses/ncurses.faq.html#config_leaks.
#
#    libtool --mode=execute valgrind --leak-check=full examples/tc1
#
# A valgrind suppressions file for ncurses is available at 
# http://www.opensource.apple.com/source/ncurses/ncurses-27/ncurses/misc/ncurses.supp
#
#    libtool --mode=execute valgrind --suppressions=ncurses.supp.txt --leak-check=full examples/tc1
#
# Verbose output can be enabled with
#    "./configure --disable-silent-rules" or "make V=1"
#

AC_INIT(libedit, [EL_RELEASE],, libedit-[EL_TIMESTAMP])
AC_CONFIG_MACRO_DIR([m4])
AC_CONFIG_SRCDIR([src/strlcat.c])
AC_CONFIG_HEADER([config.h])
AM_SILENT_RULES([yes])

# features of Posix that are extensions to C (define _GNU_SOURCE)
AC_USE_SYSTEM_EXTENSIONS

AM_INIT_AUTOMAKE
AC_PROG_LIBTOOL

# libtool -version-info
AC_SUBST(LT_VERSION, [0:51:0])

m4_ifdef([AM_SILENT_RULES], [AM_SILENT_RULES([yes])])

# Checks for programs.
AC_PROG_CC_C99
#AC_PROG_CC
AC_PROG_LN_S
AC_PROG_AWK
EL_MANTYPE


# Options for code coverage tracking
AC_ARG_ENABLE([coverage],
    AS_HELP_STRING([--enable-coverage], [Enable code coverage tracking]))
AS_IF([test "x$enable_coverage" = "xyes"], [
    AC_DEFINE([COVERAGE], [], ["Link with gcov."])
    COVERAGE_FLAGS="--coverage"
])
if test -n "$COVERAGE_FLAGS"; then
    AC_CHECK_LIB([gcov], [__gcov_init], [], [AC_MSG_ERROR([Missing gcov library])])
fi

AC_ARG_WITH([asan],
            [AS_HELP_STRING([--with-asan],
                            [Build with address sanitizer checking])],
            [with_asan=${withval}],
            [with_asan=no])
AS_IF([test "x$with_asan" = "xno"],
      [ASAN_FLAGS=],
      [test "x$with_asan" = "xyes"],
      [ASAN_FLAGS="-fsanitize=address -fsanitize-recover=address"],
      [ASAN_FLAGS="$with_asan"])
AC_SUBST([AM_CFLAGS], ["$ASAN_FLAGS $AM_CFLAGS $COVERAGE_FLAGS"])
AC_SUBST([AM_LDFLAGS], ["$COVERAGE_FLAGS -L$lt_sysroot/\$(libdir)"])

AC_SUBST([AM_CPPFLAGS], ["$AM_CPPFLAGS -I$lt_sysroot/\$(prefix)/include"])
### use option --enable-widec to turn on use of wide-character support
EL_ENABLE_WIDEC

AC_ARG_ENABLE(
  [examples],
  [AS_HELP_STRING([--enable-examples], [build the example programs [default=no]])],
  [enable_examples="$enableval"],
  [enable_examples="no"]
)

AM_CONDITIONAL(ENABLE_EXAMPLES, [test "$enable_examples" == "yes"])

# Checks for header files.
AC_HEADER_DIRENT
AC_HEADER_STDC
AC_HEADER_SYS_WAIT
AC_CHECK_HEADERS([fcntl.h limits.h malloc.h stdlib.h string.h sys/ioctl.h sys/param.h unistd.h curses.h ncurses.h sys/cdefs.h termcap.h])

AC_CHECK_HEADER([termios.h], [], [AC_MSG_ERROR([termios.h is required!])],[])

## include curses.h to prevent "Present But Cannot Be Compiled"
AC_CHECK_HEADERS([term.h],,,
[[#if HAVE_CURSES_H
# include <curses.h>
#elif HAVE_NCURSES_H
# include <ncurses.h>
#endif
]])

# Check for dirent.d_namlen field explicitly
# (This is a bit more straightforward than, if not quite as portable as,
# the recipe given by the autoconf maintainers.)
AC_CHECK_MEMBER(struct dirent.d_namlen,
AC_DEFINE([HAVE_STRUCT_DIRENT_D_NAMLEN],[1],
[Define to 1 if struct dirent has member d_namlen]),,
[#if HAVE_DIRENT_H
#include <dirent.h>
#endif
])

# Checks for typedefs, structures, and compiler characteristics.
AC_C_CONST
AC_TYPE_PID_T
AC_TYPE_SIZE_T
AC_CHECK_TYPES([u_int32_t])

# Checks for library functions.
AC_FUNC_CLOSEDIR_VOID
AC_FUNC_FORK
AC_PROG_GCC_TRADITIONAL
## _AIX is offended by rpl_malloc and rpl_realloc
#AC_FUNC_MALLOC
#AC_FUNC_REALLOC
AC_TYPE_SIGNAL
AC_FUNC_STAT
AC_CHECK_FUNCS([endpwent isascii memchr memset re_comp regcomp strcasecmp strchr strcspn strdup strerror strrchr strstr strtol issetugid wcsdup strlcpy strlcat fgetln vis strvis strunvis __secure_getenv secure_getenv])

# strlcpy
AC_CHECK_FUNC(strlcpy, found_strlcpy=yes, found_strlcpy=no)
AM_CONDITIONAL(HAVE_STRLCPY, [test "x$found_strlcpy" = xyes])

# strlcat
AC_CHECK_FUNC(strlcat, found_strlcat=yes, found_strlcat=no)
AM_CONDITIONAL(HAVE_STRLCAT, [test "x$found_strlcat" = xyes])

# vis
AC_CHECK_FUNC(vis, found_vis=yes, found_vis=no)
AM_CONDITIONAL(HAVE_VIS, [test "x$found_vis" = xyes])

# unvis
AC_CHECK_FUNC(unvis, found_unvis=yes, found_unvis=no)
AM_CONDITIONAL(HAVE_UNVIS, [test "x$found_unvis" = xyes])


EL_GETPW_R_POSIX
EL_GETPW_R_DRAFT


AH_BOTTOM([
#include "sys.h"
#define SCCSID
#undef LIBC_SCCS
#define lint
])

AC_CONFIG_FILES([Makefile
                 libedit.pc
                 src/Makefile
                 doc/Makefile
                 examples/Makefile])
AC_OUTPUT
