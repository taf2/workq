AC_PREREQ(2.59)
AC_INIT(workqd.cc)
WORKQ_RELEASE=1.1.1
WORKQ_LIBRARY_VERSION=1:0:0
AM_INIT_AUTOMAKE(workq,1.1.1)
AC_CONFIG_HEADER(workq/config.h)

AC_PROG_CXX
AC_LANG_CPLUSPLUS
AC_PROG_INSTALL
AC_PROG_LIBTOOL
AC_CONFIG_MACRO_DIR(m4)

AC_SUBST(WORKQ_RELEASE)
AC_SUBST(WORKQ_LIBRARY_VERSION)

CPPFLAGS="$CPPFLAGS -Wall "

dnl check for glib
PKG_CHECK_MODULES(GLIB2, gmodule-2.0 gobject-2.0 gthread-2.0 glib-2.0 >= 2.10.0 ,,[echo "Missing glib-2.0 and gthread-2.0, check PKG_CONFIG_PATH"; exit])
CXXFLAGS="$CXXFLAGS $GLIB2_CFLAGS"
CPPFLAGS="$CPPFLAGS $GLIB2_CFLAGS"
CXXLINK="$CXXLINK $GLIB2_LIBS"
LDFLAGS="$LDFLAGS $GLIB2_LIBS"

GTK_DOC_CHECK

m4_include([m4/mysql_client.m4])

dnl check for mysql
AX_LIB_MYSQL
old_cflags="$CFLAGS"
old_ldflags="$LDFLAGS"
CPPFLAGS="$MYSQL_CFLAGS $CPPFLAGS"
LDFLAGS="$MYSQL_LDFLAGS $LDFLAGS"

AC_CHECK_HEADERS(mysql/mysql.h,AC_CHECK_LIB(mysqlclient_r, mysql_init, [have_mysql_mysql_h=1]))
if test "$have_mysql_mysql_h" != "1"; then
  AC_CHECK_HEADERS(mysql.h,AC_CHECK_LIB(mysqlclient_r, mysql_init, [have_mysql_h=1]))
  if test "$have_mysql_h" != "1"; then
    echo "Failed to detect mysql libaries.  Make sure you have the mysql development libaries installed and try again with ./configure --with-mysql=/path/to/your/mysql_config"
    exit 1 
  fi
fi
CPPFLAGS="$old_cflags"
LDFLAGS="$old_ldflags"

AC_CHECK_HEADERS(ev.h,,[echo "Missing libev, see: http://software.schmorp.de/pkg/libev.html"; exit],
[[#if HAVE_EV_H
# include <ev.h>
#endif
]])
AC_CHECK_FUNC(ev_loop, , AC_CHECK_LIB(ev, ev_loop))

AC_CHECK_HEADERS(yaml.h,,[echo "Missing libyaml, see: http://pyyaml.org/wiki/LibYAML "; exit],
[[#if HAVE_YAML_H
# include <yaml.h>
#endif
]])
AC_CHECK_FUNC(yaml_parser_initialize, , AC_CHECK_LIB(yaml, yaml_parser_initialize))

dnl check for json-glib
PKG_CHECK_MODULES(JSON_GLIB, json-glib-1.0 ,,[echo "Missing json-glib library, see: http://live.gnome.org/JsonGlib"; exit])

dnl no exceptions
CXXFLAGS="$CXXFLAGS -fno-exceptions"

echo "CXXFLAGS: ${CXXFLAGS}"
echo "CPPFLAGS: ${CPPFLAGS}"
echo "LDFLAGS: ${LDFLAGS}"
echo "CXXLINK: ${CXXLINK}"
AC_OUTPUT(Makefile workq/Makefile modules/Makefile \
          tests/Makefile docs/Makefile docs/reference/Makefile \
          docs/reference/workq/Makefile docs/reference/modules/Makefile \
          workq-1.0.pc)
