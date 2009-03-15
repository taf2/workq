#!/bin/sh

autoheader
aclocal -I m4
gprefix=`which glibtoolize 2>&1 >/dev/null`
if [ $? -eq 0 ]; then 
  glibtoolize --force
else
  libtoolize --force
fi
autoconf
gtkdocize
automake --force --add-missing
