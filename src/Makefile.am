## Process this file with automake to produce Makefile.in.
## Makefile.am for hello/src.
##
## Copyright (C) 1996, 1997, 2001, 2005, 2006 Free Software Foundation, Inc.
##
## This program is free software; you can redistribute it and/or modify
## it under the terms of the GNU General Public License as published by
## the Free Software Foundation; either version 2, or (at your option)
## any later version.
##
## This program is distributed in the hope that it will be useful,
## but WITHOUT ANY WARRANTY; without even the implied warranty of
## MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
## GNU General Public License for more details.
##
## You should have received a copy of the GNU General Public License
## along with this program; if not, write to the Free Software Foundation,
## Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.

bin_PROGRAMS = hello
hello_SOURCES = hello.c system.h

hello_LDADD = @LIBINTL@ ../gnulib/lib/libgnu.a

localedir = $(datadir)/locale

AM_CPPFLAGS = -I$(top_srcdir)/gnulib/lib -I$(top_builddir)/gnulib/lib
DEFS = -DLOCALEDIR=\"$(localedir)\" @DEFS@
