dnl Process this file with autoconf to produce a configure script.
dnl
dnl This file is free software; as a special exception the author gives
dnl unlimited permission to copy and/or distribute it, with or without
dnl modifications, as long as this notice is preserved.
dnl
dnl This program is distributed in the hope that it will be useful, but
dnl WITHOUT ANY WARRANTY, to the extent permitted by law; without even the
dnl implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

AC_INIT([GNU html2db], [0.1], [bug-html2db@gnu.org])

dnl Must come before AM_INIT_AUTOMAKE.
AC_CONFIG_AUX_DIR([build-aux])
AM_INIT_AUTOMAKE([dist-bzip2 readme-alpha])

# Minimum Autoconf version required.
AC_PREREQ(2.60)

# Where to generate output; srcdir location.
AC_CONFIG_HEADERS([config.h:config.h.in])
AC_CONFIG_SRCDIR([src/main.c])

dnl Checks for programs.
# We need a C compiler.
AC_PROG_CC

AC_GNU_SOURCE
AM_MAINTAINER_MODE

# Since we use gnulib: gl_EARLY must be called as soon as possible after
# the C compiler is checked.  The others could be later, but we just
# keep everything together.
gl_EARLY
gl_INIT

# GNU help2man creates man pages from --help output; in many cases, this
# is sufficient, and obviates the need to maintain man pages separately.
# However, this means invoking executables, which we generally cannot do
# when cross-compiling, so we test to avoid that (the variable
# "cross_compiling" is set by AC_PROG_CC).
if test $cross_compiling = no; then
  AM_MISSING_PROG(HELP2MAN, help2man)
else
  HELP2MAN=:
fi

# i18n support from GNU gettext.
AM_GNU_GETTEXT_VERSION([0.16])
AM_GNU_GETTEXT([external])

AC_CONFIG_FILES([Makefile
           doc/Makefile
           man/Makefile
           src/Makefile
           tests/Makefile])
AC_OUTPUT
