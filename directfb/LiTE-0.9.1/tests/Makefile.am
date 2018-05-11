INCLUDES = \
	-I$(top_srcdir)	\
	@DFB_CFLAGS@

LIBS = \
	$(top_builddir)/leck/libleck.la	\
	$(top_builddir)/lite/liblite.la	\
	@DFB_LIBS@


noinst_PROGRAMS = \
	nothemewindow	\
	window		\
	utcursor	\
	utlabel		\
	utslider

nothemewindow_SOURCES = nothemewindow.c testutils.c testutils.h
nothemewindow_LDADD   = $(LIBS)

window_SOURCES        = window.c testutils.c testutils.h
window_LDADD          = $(LIBS)

utcursor_SOURCES      = utcursor.c testutils.c testutils.h
utcursor_LDADD        = $(LIBS)

utlabel_SOURCES       = utlabel.c testutils.c testutils.h
utlabel_LDADD         = $(LIBS)

utslider_SOURCES      = utslider.c testutils.c testutils.h
utslider_LDADD        = $(LIBS)

