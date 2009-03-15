dnl @synopsis AX_UPLOAD([command])
dnl
dnl Adds support for uploading dist files. %%s in the command will be
dnl substituted with the name of the file. e.g:
dnl
dnl    AX_UPLOAD([ncftpput -v upload.sourceforge.net /incoming %%s])
dnl
dnl To add upload support for other custom dists add upload-<TYPE> to
dnl UPLOAD_BIN or UPLOAD_SRC, where <TYPE> is the type of dist that is
dnl being uploaded and add a mapping from <TYPE> to the dist file name
dnl in the format '{<TYPE>=><FILENAME>}' to UPLOAD_TARGETS. For
dnl example:
dnl
dnl    UPLOAD_BIN += upload-foobar
dnl    UPLOAD_TARGETS += {foobar=>@PACKAGE@-@VERSION@.fb}
dnl
dnl You can then upload of the src distribution files by running:
dnl
dnl    make upload-src
dnl
dnl all the binaru distribution files by running:
dnl
dnl    make upload-bin
dnl
dnl or both by running:
dnl
dnl    make upload
dnl
dnl @category Automake
dnl @author Tom Howard <tomhoward@users.sf.net>
dnl @version 2005-01-14
dnl @license AllPermissive

AC_DEFUN([AX_UPLOAD],
[
AC_MSG_NOTICE([adding upload support])
AM_CONDITIONAL(USING_AX_UPLOAD, [true])
AC_MSG_NOTICE([setting upload command... \`$1\`])
AX_ADD_AM_MACRO([[
UPLOAD_BIN =
UPLOAD_SRC = upload-gzip upload-bzip2 upload-zip
UPLOAD_TARGETS = \\
{gzip=>$PACKAGE-$VERSION.tar.gz} \\
{bzip2=>$PACKAGE-$VERSION.tar.bz2} \\
{zip=>$PACKAGE-$VERSION.zip}

\$(UPLOAD_BIN) \$(UPLOAD_SRC):
	@TYPE=\`echo ${AX_DOLLAR}@ | \$(SED) -e \'s/upload-//\'\`; \\
	DIST=\"dist-\$\${TYPE}\"; \\
	\$(MAKE) \$(AM_MAKEFLAGS) \$\${DIST}; \\
	list=\'\$(UPLOAD_TARGETS)\'; \\
	pattern=\`echo \"^{\$\${TYPE}=>\"\`; \\
	for dist in \$\$list; do \\
		echo \$\$dist | \$(EGREP) \"^{\$\${TYPE}=>\" > /dev/null 2>&1; \\
		if test \"\$\$?\" -eq \"0\"; then \\
			TARGET=\`echo \"\$\$dist\" | \$(AWK) -v pattern=\$\$pattern \'{ sub( pattern, \"\"); sub( /}\$\$/, \"\" ); print; }\'\`; \\
			UPLOAD_COMMAND=\`printf \"$1\" \$\$TARGET \`; \\
			echo \"Uploading \$\$TARGET ...\"; \\
			\$\$UPLOAD_COMMAND; \\
		fi \\
	done

upload-src: \$(UPLOAD_SRC)

upload-bin: \$(UPLOAD_BIN)

upload upload-all all-upload: upload-src upload-bin
]])
])
