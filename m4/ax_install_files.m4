dnl @synopsis AX_INSTALL_FILES
dnl
dnl Adds target for creating a install_files file, which contains the
dnl list of files that will be installed.
dnl
dnl @category Automake
dnl @author Tom Howard <tomhoward@users.sf.net>
dnl @version 2005-01-14
dnl @license AllPermissive

AC_DEFUN([AX_INSTALL_FILES],
[
AC_MSG_NOTICE([adding install_files support])
AC_ARG_VAR(GAWK, [gawk executable to use])
if test "x$GAWK" = "x"; then
   AC_CHECK_PROGS(GAWK,[gawk])
fi

if test "x$GAWK" != "x"; then
   AC_MSG_NOTICE([install_files support enabled])
   AX_HAVE_INSTALL_FILES=true
   AX_ADD_AM_MACRO([[
CLEANFILES += \\
\$(top_builddir)/install_files

\$(top_builddir)/install_files: do-mfstamp-recursive
	@if test \"\$(top_builddir)/mfstamp\" -nt \"\$(top_builddir)/install_files\"; then \\
	cd \$(top_builddir) && STAGING=\"\$(PWD)/staging\"; \\
	\$(MAKE) \$(AM_MAKEFLAGS) DESTDIR=\"\$\$STAGING\" install; \\
	cd \"\$\$STAGING\" && find "." ! -type d -print | \\
	$GAWK \' \\
	    /^\\.\\/usr\\/local\\/lib/ { \\
	        sub( /\\.\\/usr\\/local\\/lib/, \"%%{_libdir}\" ); } \\
	    /^\\.\\/usr\\/local\\/bin/ { \\
	        sub( /\\.\\/usr\\/local\\/bin/, \"%%{_bindir}\" ); } \\
	    /^\\.\\/usr\\/local\\/include/ { \\
		sub( /\\.\\/usr\\/local\\/include/, \"%%{_includedir}\" ); } \\
	    /^\\.\\/usr\\/local\\/share/ { \\
	        sub( /\\.\\/usr\\/local\\/share/, \"%%{_datadir}\" ); } \\
	    /^\\.\\/usr\\/local/ { \\
		sub( /\\.\\/usr\\/local/, \"%%{_prefix}\" ); } \\
	    /^\\./ { sub( /\\./, \"\" ); } \\
	    /./ { print; }\' > ../install_files; \\
	rm -rf \"\$\$STAGING\"; \\
	else \\
	    echo \"\\\`\$(top_builddir)/install_files\' is up to date.\"; \\
	fi

]])
    AX_ADD_RECURSIVE_AM_MACRO([do-mfstamp],[[
\$(top_builddir)/mfstamp:  do-mfstamp-recursive

do-mfstamp-am do-mfstamp: Makefile.in
	@echo \"timestamp for all Makefile.in files\" > \$(top_builddir)/mfstamp
	@touch ${AX_DOLLAR}@

]])
else
    AX_HAVE_INSTALL_FILES=false;
    AC_MSG_WARN([install_files support disable... gawk not found])
fi
])# AX_INSTALL_FILES
