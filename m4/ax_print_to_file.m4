dnl @synopsis AX_PRINT_TO_FILE([FILE],[DATA])
dnl
dnl Writes the specified data to the specified file.
dnl
dnl @category Automake
dnl @author Tom Howard <tomhoward@users.sf.net>
dnl @version 2005-01-14
dnl @license AllPermissive

AC_DEFUN([AX_PRINT_TO_FILE],[
AC_REQUIRE([AX_FILE_ESCAPES])
printf "$2" > "$1"
])
