AC_DEFUN([AX_MAGICK_CORE],
[
  AC_ARG_WITH([magick-core],
    AC_HELP_STRING([--with-magickcore=@<:@ARG@:>@],
        [use ImageMagick Core library @<:@default=yes@:>@, optionally specify path to MagickCore-config]
    ),
    [
    if test "$withval" = "no"; then
      want_magickcore="no"
    elif test "$withval" = "yes"; then
      want_magickcore="yes"
    else
      want_magickcore="yes"
      MAGICKCORE_CONFIG="$withval"
    fi
    ],
    [want_magickcore="yes"]
  )

  MAGICKCORE_CFLAGS=""
  MAGICKCORE_LDFLAGS=""
  MAGICKCORE_VERSION=""
  if test "$want_magickcore" = "yes"; then
    if test -z "$MAGICKCORE_CONFIG" -o test; then
      AC_PATH_PROG([MAGICKCORE_CONFIG], [MagickCore-config], [no])
    fi
    if test "$MAGICKCORE_CONFIG" != "no"; then
      AC_MSG_CHECKING([for ImageMagick Core libraries])
      MAGICKCORE_CFLAGS="`$MAGICKCORE_CONFIG --cflags --cppflags`"
      MAGICKCORE_LDFLAGS="`$MAGICKCORE_CONFIG --ldflags --libs`"
      MAGICKCORE_CFLAGS=$(echo $MAGICKCORE_CFLAGS)
      MAGICKCORE_LDFLAGS=$(echo $MAGICKCORE_LDFLAGS)

      MAGICKCORE_VERSION="`$MAGICKCORE_CONFIG --version`"
      MAGICKCORE_VERSION=$(echo $MAGICKCORE_VERSION)

      AC_DEFINE([HAVE_MAGICKCORE], [1],
          [Define to 1 if ImageMagick Core libraries are available])

      found_magickcore="yes"
      AC_MSG_RESULT([yes])
    else
      found_magickcore="no"
      AC_MSG_RESULT([no])
    fi
  fi

  dnl TODO: Check for availability of required ImageMagick version

  AC_SUBST([MAGICKCORE_VERSION])
  AC_SUBST([MAGICKCORE_CFLAGS])
  AC_SUBST([MAGICKCORE_LDFLAGS])
])
