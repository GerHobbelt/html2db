# Process this file with automake to produce Makefile.in.
# Makefile for hello/tests.
#
# Copyright (C) 1996, 2005, 2006 Free Software Foundation, Inc.
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2, or (at your option)
# any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software Foundation,
# Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.

TESTS = greeting-1 greeting-2 hello-1 traditional-1

EXTRA_DIST = $(TESTS)

TESTS_ENVIRONMENT = top_srcdir=$(top_srcdir) PATH=.:../src:$$PATH \
		    HELLO=`echo hello|sed '$(transform)'` \
		    $(SHELL)
