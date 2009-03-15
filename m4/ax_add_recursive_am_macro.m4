dnl @synopsis AX_ADD_RECURSIVE_AM_MACRO([TARGET],[RULE])
dnl
dnl Adds the specified rule to $AMINCLUDE along with a TARGET-recursive
dnl rule that will call TARGET for the current directory and TARGET-am
dnl recursively for each subdirectory
dnl
dnl @category Automake
dnl @author Tom Howard <tomhoward@users.sf.net>
dnl @version 2005-01-14
dnl @license AllPermissive

AC_DEFUN([AX_ADD_RECURSIVE_AM_MACRO],[
  AX_ADD_AM_MACRO([
$1-recursive:
	@set fnord ${AX_DOLLAR}${AX_DOLLAR}MAKEFLAGS; amf=${AX_DOLLAR}${AX_DOLLAR}2; \\
	dot_seen=no; \\
	list='${AX_DOLLAR}(SUBDIRS)'; for subdir in ${AX_DOLLAR}${AX_DOLLAR}list; do \\
	  echo \"Making $1 in ${AX_DOLLAR}${AX_DOLLAR}subdir\"; \\
	  if test \"${AX_DOLLAR}${AX_DOLLAR}subdir\" = \".\"; then \\
	    dot_seen=yes; \\
	    local_target=\"$1-am\"; \\
	  else \\
	    local_target=\"$1\"; \\
	  fi; \\
	  (cd ${AX_DOLLAR}${AX_DOLLAR}subdir && ${AX_DOLLAR}(MAKE) ${AX_DOLLAR}(AM_MAKEFLAGS) ${AX_DOLLAR}${AX_DOLLAR}local_target) \\
	   || case \"${AX_DOLLAR}${AX_DOLLAR}amf\" in *=*) exit 1;; *k*) fail=yes;; *) exit 1;; esac; \\
	done; \\
	if test \"${AX_DOLLAR}${AX_DOLLAR}dot_seen\" = \"no\"; then \\
	  ${AX_DOLLAR}(MAKE) ${AX_DOLLAR}(AM_MAKEFLAGS) \"$1-am\" || exit 1; \\
	fi; test -z \"${AX_DOLLAR}${AX_DOLLAR}fail\"

$2
])
])
