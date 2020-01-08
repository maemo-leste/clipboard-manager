AC_INIT(clipboard-manager, 0.6.8)
AC_CONFIG_AUX_DIR(config)

AM_INIT_AUTOMAKE()
AM_MAINTAINER_MODE

AC_PROG_CC


PKG_CHECK_MODULES(X, x11, [have_x=yes], [have_x=no])
PKG_CHECK_MODULES(CM_DEPS,
	[gconf-2.0 >= 2.16.0
	 glib-2.0 >= 2.12.0
	 gobject-2.0
	])

if test x$have_x = xno ; then
  AC_PATH_XTRA
fi

if test x$have_x = xno ; then
  AC_MSG_ERROR([X development libraries not found])
fi

AC_SUBST(X_CFLAGS)
AC_SUBST(X_LIBS)
AC_DEFINE_UNQUOTED(PACKAGE_SYSCONFDIR,"${sysconfdir}")

AC_OUTPUT([
        Makefile
        debian/Makefile
])
