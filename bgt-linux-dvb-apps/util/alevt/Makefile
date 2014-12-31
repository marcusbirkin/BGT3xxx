VER=1.7.0
OPT=-O -g -w
DEFS=-DWITH_PNG
DEFS+=-DUSE_LIBZVBI
FONT=vtxt
MAN_DIR=man
DESTDIR=
PREFIX=/usr
HOSTCC=$(CC)
CFLAGS=$(OPT) -DVERSION=\"$(VER)\" $(DEFS) -I$(USR_X11R6)/include
EXPOBJS=export.o exp-txt.o exp-html.o exp-gfx.o font.o
OBJS=main.o ui.o xio.o fdset.o vbi.o cache.o help.o search.o misc.o hamm.o lang.o $(EXPOBJS)
TOBJS=alevt-date.o vbi.o fdset.o misc.o hamm.o lang.o
COBJS=alevt-cap.o vbi.o fdset.o misc.o hamm.o lang.o $(EXPOBJS)

ifneq ($(findstring WITH_PNG,$(DEFS)),)
EXPLIBS=-lpng -lz -lm
endif

ifneq ($(findstring USE_LIBZVBI,$(DEFS)),)
ZVBILIB=-lzvbi -lpthread
EXPLIBS+=$(ZVBILIB)
endif

all: alevt alevt-date alevt-cap alevt.1 alevt-date.1 alevt-cap.1

alevt: $(OBJS)
	$(CC) $(OPT) $(OBJS) -o alevt -L$(PREFIX)/lib -L$(PREFIX)/lib64 -lX11 $(EXPLIBS)

alevt-date: $(TOBJS)
	$(CC) $(OPT) $(TOBJS) -o alevt-date $(ZVBILIB)

alevt-cap: $(COBJS)
	$(CC) $(OPT) $(COBJS) -o alevt-cap $(EXPLIBS)

font.o: font1.xbm font2.xbm font3.xbm font4.xbm
fontsize.h: font1.xbm font2.xbm font3.xbm font4.xbm
	fgrep -h "#define" font1.xbm font2.xbm font3.xbm font4.xbm >fontsize.h

font1.xbm: bdf2xbm $(FONT)-latin-1.bdf
	./bdf2xbm font1 <$(FONT)-latin-1.bdf >font1.xbm

font2.xbm: bdf2xbm $(FONT)-latin-2.bdf
	./bdf2xbm font2 <$(FONT)-latin-2.bdf >font2.xbm

font3.xbm: bdf2xbm vtxt-koi8.bdf
	./bdf2xbm font3 <vtxt-koi8.bdf >font3.xbm

font4.xbm: bdf2xbm vtxt-iso8859-7.bdf
	./bdf2xbm font4 <vtxt-iso8859-7.bdf >font4.xbm

bdf2xbm: bdf2xbm.c
	$(HOSTCC) bdf2xbm.c -o bdf2xbm

clean:
	rm -f *.o page*.txt a.out core bdf2xbm font?.xbm fontsize.h
	rm -f alevt alevt-date alevt-cap

rpm-install: all
	install -m 0755 alevt        ${RPM_BUILD_ROOT}$(USR_X11R6)/bin
	install -m 0755 alevt-date   ${RPM_BUILD_ROOT}$(USR_X11R6)/bin
	install -m 0755 alevt-cap    ${RPM_BUILD_ROOT}$(USR_X11R6)/bin
	install -m 0644 alevt.1      ${RPM_BUILD_ROOT}$(USR_X11R6)/$(MAN)/man1
	install -m 0644 alevt-date.1 ${RPM_BUILD_ROOT}$(USR_X11R6)/$(MAN)/man1
	install -m 0644 alevt-cap.1  ${RPM_BUILD_ROOT}$(USR_X11R6)/$(MAN)/man1
	install -d 0755 $(RPM_BUILD_ROOT)$(USR_X11R6)/include/X11/pixmaps
	install -m 0644 alevt.png $(RPM_BUILD_ROOT)$(USR_X11R6)/include/X11/pixmaps

install: all
	install -m 0755 alevt		$(DESTDIR)$(PREFIX)/bin
	install -m 0755 alevt-date	$(DESTDIR)$(PREFIX)/bin
	install -m 0755 alevt-cap	$(DESTDIR)$(PREFIX)/bin
	install -m 0644 alevt.1		$(DESTDIR)$(PREFIX)/share/man/man1
	install -m 0644 alevt-date.1	$(DESTDIR)$(PREFIX)/share/man/man1
	install -m 0644 alevt-cap.1	$(DESTDIR)$(PREFIX)/share/man/man1
	install -m 0644 alevt.png $(DESTDIR)$(PREFIX)/share/pixmaps
	install -m 0644 alevt.desktop $(DESTDIR)$(PREFIX)/share/applications

uninstall: clean
	rm -f /usr/bin/alevt /usr/bin/alevt-cap /usr/bin/alevt-date \
	/usr/share/pixmaps/alevt.png /usr/share/applications/alevt.desktop \
	/usr/share/man/man1/alevt.1 /usr/share/man/man1/alevt-cap.1 \
	/usr/share/man/man1/alevt-date.1

depend:
	makedepend -Y -- $(CFLAGS_none) -- *.c 2>/dev/null

tar-html: alevt.1 alevt-date.1 alevt-cap.1
	for i in alevt.1 alevt-date.1 alevt-cap.1 ; do \
	    j=`basename $$i .1` ; \
	    j=`basename $$j .1x` ; \
	    nroff -man $$i | { \
		echo "<HTML><HEAD><TITLE>AleVT</TITLE></HEAD><BODY>" ; \
		man2html -bare -uelem U -nodepage ; \
		echo "</B0DY></HTML>" ; \
	    } | sed -e "s,</B> <B>, ,g" -e "s,</U> <U>, ,g" >~/exit/alevt/$$j.html ;\
	done

tar: tar-html clean
	sed s/VERSION/$(VER)/g <alevt.lsm.in >~/exit/alevt/alevt-$(VER).lsm
	sed s/VERSION/$(VER)/g <alevt.spec.in >alevt.spec
	cd .. ;\
	    ln -s alevt alevt-$(VER) ;\
	    tar vcfz ~/exit/alevt/alevt-$(VER).tar.gz alevt-$(VER)/* ;\
	    rm alevt-$(VER)
	cat <CHANGELOG >~/exit/alevt/changes

# DO NOT DELETE

alevt-cap.o: vt.h misc.h fdset.h dllist.h vbi.h cache.h lang.h export.h
alevt-date.o: os.h vt.h misc.h fdset.h dllist.h vbi.h cache.h lang.h
cache.o: misc.h dllist.h cache.h vt.h help.h
exp-gfx.o: lang.h misc.h vt.h export.h font.h fontsize.h
exp-html.o: lang.h misc.h vt.h export.h
exp-txt.o: os.h export.h vt.h misc.h
export.o: vt.h misc.h export.h
fdset.o: dllist.h misc.h fdset.h
font.o: font1.xbm font2.xbm font3.xbm font4.xbm
hamm.o: vt.h misc.h hamm.h
help.o: vt.h misc.h vt900.out vt901.out vt902.out vt903.out vt904.out vt905.out
help.o: vt906.out vt907.out vt908.out vt909.out vt910.out vt911.out vt912.out
lang.o: misc.h vt.h lang.h
main.o: vt.h misc.h fdset.h dllist.h xio.h vbi.h cache.h lang.h ui.h
main.o: search.h
misc.o: misc.h
search.o: vt.h misc.h cache.h dllist.h search.h
ui.o: vt.h misc.h xio.h dllist.h vbi.h cache.h lang.h fdset.h
ui.o: search.h export.h ui.h
vbi.o: os.h vt.h misc.h vbi.h dllist.h cache.h lang.h fdset.h hamm.h
xio.o: vt.h misc.h dllist.h xio.h fdset.h lang.h icon.xbm font.h fontsize.h
