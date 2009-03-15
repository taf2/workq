dnl @synopsis AX_AM_MACROS
dnl
dnl Adds support for macros that create automake rules. You must
dnl manually add the following line
dnl
dnl   @INC_AMINCLUDE@
dnl
dnl to your Makefile.am files.
dnl
dnl @category Automake
dnl @author Tom Howard <tomhoward@users.sf.net>
dnl @version 2005-01-14
dnl @license AllPermissive

AC_DEFUN([AX_AM_MACROS],
[
AC_MSG_NOTICE([adding automake macro support])
AMINCLUDE="aminclude.am"
AC_SUBST(AMINCLUDE)
AC_MSG_NOTICE([creating $AMINCLUDE])
AMINCLUDE_TIME=`date`
AX_PRINT_TO_FILE([$AMINCLUDE],[[
# generated automatically by configure from AX_AUTOMAKE_MACROS
# on $AMINCLUDE_TIME

]])

INC_AMINCLUDE="include \$(top_builddir)/$AMINCLUDE"
AC_SUBST(INC_AMINCLUDE)
])
