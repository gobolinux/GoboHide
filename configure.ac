dnl
dnl configure.in file for GoboHide
dnl

AC_INIT([gobohide], [1.0])
AC_CONFIG_SRCDIR(src/gobohide.c)
AC_CONFIG_MACRO_DIRS([m4])
AC_PROG_CC
AC_PATH_TOOL([PKGCONFIG], [pkg-config], [no])
if test  "x$PKGCONFIG" = "xno"
then
    AC_MSG_ERROR([pkg-config not found])
fi

AM_INIT_AUTOMAKE
AM_PROG_LIBTOOL
AM_GNU_GETTEXT([external])
AM_GNU_GETTEXT_VERSION(0.13.1)

PKG_CHECK_MODULES(LIBNL3, libnl-3.0 >= 3.1, [true],
    AC_MSG_ERROR([netlink library (libnl) not found]))
PKG_CHECK_MODULES(LIBNL3_GENL, libnl-genl-3.0 >= 3.1, [true],
    AC_MSG_ERROR([generic netlink library (libnl-genl) not found]))
PKG_CHECK_MODULES(LIBNL3_CLI, libnl-cli-3.0 >= 3.1, [true],
    AC_MSG_ERROR([generic netlink command line interface library (libnl-cli) not found]))

CFLAGS="$CFLAGS `$PKGCONFIG --cflags libnl-3.0 libnl-genl-3.0 libnl-cli-3.0`"
LDFLAGS="$LDFLAGS `$PKGCONFIG --libs libnl-3.0 libnl-genl-3.0 libnl-cli-3.0`"
AC_SUBST([CFLAGS])
AC_SUBST([LDFLAGS])

AC_CONFIG_FILES([Makefile])
AC_CONFIG_FILES([m4/Makefile])
AC_CONFIG_FILES([po/Makefile.in])
AC_CONFIG_FILES([man/Makefile])
AC_CONFIG_FILES([src/Makefile])
AC_OUTPUT
