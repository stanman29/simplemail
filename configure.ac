dnl Process this file with autoconf to produce a configure script.
AC_INIT(SimpleMail_rev.h)

dnl Automake stuff
AM_INIT_AUTOMAKE(SimpleMail,0.4)
AM_CONFIG_HEADER(config.h)

AC_ISC_POSIX

dnl nls
PACKAGE=simplemail
VERSION=0.11
AC_DEFINE_UNQUOTED(PACKAGE, "$PACKAGE")
AC_DEFINE_UNQUOTED(VERSION, "$VERSION")
AC_SUBST(PACKAGE)
AC_SUBST(VERSION)

dnl Checks for programs.
AC_PROG_CC
AC_PROG_RANLIB

dnl Checks for libraries.
AC_HAVE_LIBRARY(ssl)
AC_HAVE_LIBRARY(expat)

dnl Checks for header files.
AC_HEADER_STDC
AC_HEADER_DIRENT
AC_HEADER_TIME
AC_CHECK_HEADERS([alloca.h])
AC_CHECK_HEADERS([errno.h])
AC_CHECK_HEADERS([fcntl.h])
AC_CHECK_HEADERS([libintl.h])
AC_CHECK_HEADERS([netdb.h])
AC_CHECK_HEADERS([netinet/in.h])
AC_CHECK_HEADERS([stddef.h])
AC_CHECK_HEADERS([sys/socket.h])
AC_CHECK_HEADERS([sys/time.h])

dnl Checks for typedefs, structures, and compiler characteristics.
AC_C_BIGENDIAN
AC_C_CONST
AC_TYPE_SIZE_T
AC_STRUCT_TM

dnl Checks for library functions.
AM_PATH_GTK_2_0([],[],[],"gthread")
PKG_CHECK_MODULES(LIBGTKHTML, libgtkhtml-2.0 >= 2.0.0)
AC_FUNC_MEMCMP
AC_FUNC_MALLOC
AC_FUNC_MKTIME
AC_FUNC_STAT
AC_CHECK_FUNCS(getcwd gethostname socket strdup strstr strtol strcasecmp strncasecmp)
AC_CHECK_FUNCS([bzero])
AC_CHECK_FUNCS([gethostbyname])
AC_CHECK_FUNCS([gettimeofday])
AC_CHECK_FUNCS([getwd])
AC_CHECK_FUNCS([memchr])
AC_CHECK_FUNCS([memmove])
AC_CHECK_FUNCS([mempcpy])
AC_CHECK_FUNCS([memset])
AC_CHECK_FUNCS([mkdir])
AC_CHECK_FUNCS([strerror])
AC_CHECK_FUNCS([strndup])
AC_CHECK_FUNCS([strrchr])
AC_CHECK_FUNCS([strtoul])

dnl I18n support
ALL_LINGUAS="de ma"
AM_GNU_GETTEXT

AC_PROG_LN_S

CFLAGS="$CFLAGS -Wall"
GUI_CFLAGS="$GTK_CFLAGS $GLIB_CFLAGS $LIBGTKHTML_CFLAGS"
GUI_LIBS="$GTK_LIBS $GLIB_LIBS $LIBGTKHTML_LIBS"
AC_SUBST(GUI_CFLAGS)
AC_SUBST(GUI_LIBS)

AC_OUTPUT(Makefile
          gtk/Makefile
          intl/Makefile
          po/Makefile.in)

