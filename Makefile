# Makefile
#
# wifimgr
#	manage WiFi networks
#
# $Id: Makefile 57 2009-12-18 19:29:30Z jr $
#

all:
	cd src; ${MAKE}

clean:
	cd src; ${MAKE} clean

install:
	cd src; ${MAKE} install

deinstall:
	cd src; ${MAKE} deinstall

PKG =	\
	Authors \
	ChangeLog \
	LICENSE \
	LICENSE-psdGraphics.com \
	Makefile \
	ReadMe \
	icons \
	icons-wm \
	man \
	po \
	src

VER != sed -n '/VERSION/s/.*"Version \([0-9][0-9a-z.]*\)"/\1/p' src/version.h

package:
	cd src; ${MAKE} clean
	tar -cyv --exclude '*/.svn' -s ",^,wifimgr-${VER}/," -f /usr/ports/distfiles/wifimgr-${VER}.tar.bz2 ${PKG}
