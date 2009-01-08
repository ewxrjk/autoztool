# $Header: /cvs/autoztool/Makefile,v 1.6 2003-08-18 22:30:28 richard Exp $

#
# This file is part of autoztool
# Copyright (C) 2001, 2003 Richard Kettlewell
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

prefix=/usr/local
bindir=${prefix}/bin
libdir=${prefix}/lib
mandir=${prefix}/man
man1dir=${mandir}/man1
pkglibdir=${libdir}/autoztool

VERSION=0.2

CC=gcc
CFLAGS=-Wall -W
INSTALL=install -c

all: autoztool.so z

z: z.m4 Makefile
	m4 -Dpkglibdir="${pkglibdir}" -DVERSION="${VERSION}" z.m4 > z.tmp
	mv z.tmp z
	chmod 755 z

autoztool.so: autoztool.lo
	gcc -shared -o autoztool.so autoztool.lo -ldl -lc

install:
	@set -e;if test ! -d $(libdir)/autoztool; then \
		echo mkdir -p $(libdir)/autoztool; \
		mkdir -p $(libdir)/autoztool; \
	fi
	$(INSTALL) -m 755 z $(bindir)/z
	$(INSTALL) -m 644 autoztool.so $(libdir)/autoztool/autoztool.so 
	$(INSTALL) -m 644 z.1 $(man1dir)/z.1

install-strip: install

uninstall:
	rm -f $(bindir)/z
	rm -f $(libdir)/autoztool/autoztool.so
	rm -f $(man1dir)/z.1

installdirs:
	mkdir -p $(libdir)
	mkdir -p $(bindir)
	mkdir -p $(man1dir)

clean:
	rm -f *.so
	rm -f *.lo
	rm -f z

dist:
	rm -rf autoztool-${VERSION}
	mkdir autoztool-${VERSION}
	cp COPYING Makefile README *.c z.m4 z.1 autoztool-${VERSION} 
	mkdir autoztool-${VERSION}/debian
	cp debian/autorules.m4 debian/changelog autoztool-${VERSION}/debian
	cp debian/control debian/copyright autoztool-${VERSION}/debian
	cp debian/rules.m4 debian/rules autoztool-${VERSION}/debian
	chmod +x autoztool-${VERSION}/debian/rules
	tar cf autoztool-${VERSION}.tar autoztool-${VERSION}
	gzip -9vf autoztool-${VERSION}.tar 

%.lo : %.c
	$(CC) $(CPPFLAGS) $(CFLAGS) -fpic -c $< -o $@
