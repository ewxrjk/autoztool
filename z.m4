#! /bin/sh
#
# This file is part of autoztool
# Copyright (C) 2001, 2003, 2010 Richard Kettlewell
# 
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.
# 
# This program is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
# General Public License for more details.
# 
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307
# USA
# 

set -e

case "$1" in
-h | --help )
  echo "autoztool VERSION"
  echo
  echo "Usage:"
  echo
  echo "  z [--] COMMAND ARGS..."
  echo
  exit 0
  ;;
-V | --version )
  echo "autoztool VERSION"
  exit 0
  ;;
-- )
  shift
  ;;
-* )
  echo "unknown option '$1'" 1>&2
  exit 1
  ;;
esac

if test "x$LD_PRELOAD" = "x"; then
  LD_PRELOAD=pkglibdir/autoztool.so
else
  LD_PRELOAD="pkglibdir/autoztool.so:$LD_PRELOAD"
fi
export LD_PRELOAD
exec "$@"
