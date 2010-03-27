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

prefix=/usr/local
bindir=${prefix}/bin
libdir=${prefix}/lib
mandir=${prefix}/man
man1dir=${mandir}/man1
pkglibdir=${libdir}/autoztool

include defs.$(shell uname -s)

VERSION=0.3+

INSTALL=install -c

all: ${MODULE} z

z: z.m4 Makefile
	m4 -Dpkglibdir="${pkglibdir}" \
		-DVERSION="${VERSION}" \
		-D__module__=${MODULE} \
		-D__variable__=${VARIABLE} \
		z.m4 > z.tmp
	mv z.tmp z
	chmod 755 z

z.check: z.m4 Makefile
	m4 -Dpkglibdir=`pwd` \
		-DVERSION="${VERSION}" \
		-D__module__=${MODULE} \
		-D__variable__=${VARIABLE} \
		z.m4 > $@.tmp
	mv $@.tmp $@
	chmod 755 $@

$(MODULE): autoztool.lo
	$(CC) $(CFLAGS) $(SHAREFLAGS) -o $@ $^ $(LIBS)

install: installdirs
	$(INSTALL) -m 755 z $(bindir)/z
	$(INSTALL) -m 644 ${MODULE} $(libdir)/autoztool/${MODULE} 
	$(INSTALL) -m 644 z.1 $(man1dir)/z.1

install-strip: install

uninstall:
	rm -f $(bindir)/z
	rm -f $(libdir)/autoztool/${MODULE}
	rm -f $(man1dir)/z.1
	-rmdir $(libdir)/autoztool

installdirs:
	mkdir -p $(libdir)
	mkdir -p $(libdir)/autoztool
	mkdir -p $(bindir)
	mkdir -p $(man1dir)

clean:
	rm -f *.so
	rm -f *.lo
	rm -f *.dylib
	rm -f z
	rm -f z.check
	rm -f testfile testfile.gz testoutput

check: all z.check
	rm -f testfile testfile.gz
	cp z.m4 testfile
	gzip -9 testfile
	./z.check cat testfile.gz > testoutput
	cmp z.m4 testoutput
	rm -f testfile testfile.gz testoutput

dist:
	rm -rf autoztool-${VERSION}
	mkdir autoztool-${VERSION}
	cp COPYING Makefile README *.c z.m4 z.1 autoztool-${VERSION} 
	cp defs.Linux defs.Darwin autoztool-${VERSION}
	mkdir autoztool-${VERSION}/debian
	cp debian/changelog autoztool-${VERSION}/debian
	cp debian/control debian/copyright autoztool-${VERSION}/debian
	cp debian/rules autoztool-${VERSION}/debian
	chmod +x autoztool-${VERSION}/debian/rules
	tar cf autoztool-${VERSION}.tar autoztool-${VERSION}
	gzip -9vf autoztool-${VERSION}.tar
	rm -rf autoztool-${VERSION}

distcheck: dist
	tar xfz autoztool-${VERSION}.tar.gz
	cd autoztool-${VERSION} && make check
	cd autoztool-${VERSION} && make install prefix=distcheck/usr/local
	rm -rf autoztool-${VERSION}

%.lo : %.c
	$(CC) $(CPPFLAGS) $(CFLAGS) -fpic -c $< -o $@

echo-version:
	@echo "$(VERSION)"
