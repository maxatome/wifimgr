# Makefile
#
# wifimgr
#	manage WiFi networks
#
# $Id: Makefile 104 2014-05-10 22:54:38Z jr $
#

GTK_CFLAGS !=	pkg-config --cflags gtk+-2.0
GTK_LDFLAGS !=	pkg-config --libs gtk+-2.0

.if defined(WITHOUT_NLS)
CFLAGS +=	-DWITHOUT_NLS
.else
LDFLAGS +=	-L${PREFIX}/lib -lintl
.endif

CFLAGS +=	${GTK_CFLAGS}
LDFLAGS +=	${GTK_LDFLAGS}

DESTDIR ?=
PREFIX ?=	/usr/local
BASENAME ?=	basename
DIRNAME ?=	dirname
GTK_UPDATE_ICON_CACHE ?= gtk-update-icon-cache
GZIP ?=		gzip
MKDIR ?=	mkdir -p

icons =		${DESTDIR}${PREFIX}/share/wifimgr/icons/eye.png \
		${DESTDIR}${PREFIX}/share/wifimgr/icons/freebsd-surf.png \
		${DESTDIR}${PREFIX}/share/wifimgr/icons/on-off.png \
		${DESTDIR}${PREFIX}/share/wifimgr/icons/padlock-open.png \
		${DESTDIR}${PREFIX}/share/wifimgr/icons/padlock-unk.png \
		${DESTDIR}${PREFIX}/share/wifimgr/icons/padlock-rsn.png \
		${DESTDIR}${PREFIX}/share/wifimgr/icons/padlock-wep.png \
		${DESTDIR}${PREFIX}/share/wifimgr/icons/padlock-wpa.png \
		${DESTDIR}${PREFIX}/share/wifimgr/icons/signal_bars_0.png \
		${DESTDIR}${PREFIX}/share/wifimgr/icons/signal_bars_1.png \
		${DESTDIR}${PREFIX}/share/wifimgr/icons/signal_bars_2.png \
		${DESTDIR}${PREFIX}/share/wifimgr/icons/signal_bars_3.png \
		${DESTDIR}${PREFIX}/share/wifimgr/icons/signal_bars_4.png \
		${DESTDIR}${PREFIX}/share/wifimgr/icons/signal_bars_5.png \
		${DESTDIR}${PREFIX}/share/wifimgr/icons/wifimgr.png \
		${DESTDIR}${PREFIX}/share/wifimgr/icons/wifimgr-grey.png \
		${DESTDIR}${PREFIX}/share/wifimgr/icons/wifimgr24green.png

icons-wm =	${DESTDIR}${PREFIX}/share/icons/hicolor/16x16/apps/wifimgr.png \
		${DESTDIR}${PREFIX}/share/icons/hicolor/22x22/apps/wifimgr.png \
		${DESTDIR}${PREFIX}/share/icons/hicolor/24x24/apps/wifimgr.png \
		${DESTDIR}${PREFIX}/share/icons/hicolor/32x32/apps/wifimgr.png \
		${DESTDIR}${PREFIX}/share/icons/hicolor/48x48/apps/wifimgr.png \
		${DESTDIR}${PREFIX}/share/icons/hicolor/64x64/apps/wifimgr.png \
		${DESTDIR}${PREFIX}/share/icons/hicolor/scalable/apps/wifimgr.svg

.ifndef WITHOUT_NLS
langs =		${DESTDIR}${PREFIX}/share/locale/bg/LC_MESSAGES/wifimgr.mo \
		${DESTDIR}${PREFIX}/share/locale/de/LC_MESSAGES/wifimgr.mo \
		${DESTDIR}${PREFIX}/share/locale/fr/LC_MESSAGES/wifimgr.mo \
		${DESTDIR}${PREFIX}/share/locale/ja/LC_MESSAGES/wifimgr.mo \
		${DESTDIR}${PREFIX}/share/locale/nl/LC_MESSAGES/wifimgr.mo \
		${DESTDIR}${PREFIX}/share/locale/ru/LC_MESSAGES/wifimgr.mo
.endif

installed =	${DESTDIR}${PREFIX}/bin/wifimgr \
		${DESTDIR}${PREFIX}/libexec/wifimgrsu \
		${DESTDIR}${PREFIX}/share/applications/wifimgr.desktop \
		${DESTDIR}${PREFIX}/man/man8/wifimgr.8 \
		${langs} \
		${icons} \
		${icons-wm}

all:		wifimgr wifimgrsu

wmgr_objs =	wifimgr.o string.o net_list.o wifimgr-gtk.o

wifimgr:	${wmgr_objs}
		${CC} -o $@ ${wmgr_objs} ${LDFLAGS}

wifimgr.o:	wifimgr.c wifimgr.h

string.o:	string.c

net_list.o:	net_list.c wifimgr.h

wifimgr-gtk.o:	wifimgr-gtk.c wifimgr.h version.h

wmgrsu_objs =	wifimgrsu.o string.o

wifimgrsu:	${wmgrsu_objs}
		${CC} -o $@ ${wmgrsu_objs} -lcrypt ${LDFLAGS}

wifimgrsu.o:	wifimgrsu.c wifimgr.h
		${CC} -I${PREFIX}/include -c wifimgrsu.c

clean:
		rm -f wifimgr wifimgrsu
		rm -f *.o
		rm -f *.core

install:	${installed} update-icon-cache

deinstall:	do-deinstall update-icon-cache

do-deinstall:
		rm -f ${installed}
		[ -d ${PREFIX}/share/wifimgr/icons ] && rmdir ${PREFIX}/share/wifimgr/icons || true
		[ -d ${PREFIX}/share/wifimgr ] && rmdir ${PREFIX}/share/wifimgr || true

pot:
		xgettext -kgettext -c *.c -o wifimgr.pot

${DESTDIR}${PREFIX}/bin/wifimgr:	wifimgr
		${INSTALL} $? $@

${DESTDIR}${PREFIX}/libexec/wifimgrsu:	wifimgrsu
		${INSTALL} -o root -g wheel -m 4511 $? $@

${DESTDIR}${PREFIX}/share/applications/wifimgr.desktop:	wifimgr.desktop
		${INSTALL} -m 444 $? $@

${DESTDIR}${PREFIX}/man/man8/wifimgr.8: ../man/wifimgr.8
		${INSTALL} -m 444 $? $@

.for f in ${icons}
b !=		${BASENAME} ${f}
${f}:		../icons/${b}
		@${MKDIR} `${DIRNAME} ${f}`
		${INSTALL} -m 444 $? $@
.endfor

.for f in ${icons-wm}
size !=		expr ${f} : '.*/hicolor/\([^x/]*\).*'
suffix =	png
.if ${size} == "scalable"
size =		64
suffix =	svg
.endif
${f}:		../icons-wm/wifimgr${size}.${suffix}
		@${MKDIR} `${DIRNAME} ${f}`
		${INSTALL} -m 444 $? $@
.endfor

update-icon-cache:
		@for f in ${icons-wm}; do \
			if [ ! -e $$f -o $$f -nt ${PREFIX}/share/icons/hicolor/icon-theme.cache ]; then \
				echo "Updating Gtk icon cache..."; \
				${GTK_UPDATE_ICON_CACHE} ${PREFIX}/share/icons/hicolor; \
				exit 0; \
			fi; \
		done

.for f in ${langs}
l != expr "$f" : '.*/locale/\(.*\)/LC_MESSAGES/'
${f}:		../po/${l}.po
		msgfmt -o $@ $?
.endfor
