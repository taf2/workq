dnl @synopsis AX_ADD_AM_MACRO([RULE])
dnl
dnl Adds the specified rule to $AMINCLUDE
dnl
dnl @category Automake
dnl @author Tom Howard <tomhoward@users.sf.net>
dnl @version 2005-01-14
dnl @license AllPermissive

AC_DEFUN([AX_ADD_AM_MACRO],[
  AC_REQUIRE([AX_AM_MACROS])
  AX_APPEND_TO_FILE([$AMINCLUDE],[$1])
])
