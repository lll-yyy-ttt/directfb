leckincludedir = $(includedir)/leck

INCLUDES = \
	-I$(top_srcdir)	\
	@DFB_CFLAGS@

lib_LTLIBRARIES = libleck.la

libleck_la_SOURCES =	\
	animation.c	\
	button.c	\
	check.c		\
	image.c		\
	label.c		\
	list.c		\
	progressbar.c	\
	scrollbar.c	\
	slider.c	\
	textbutton.c	\
	textline.c

#	hantextline.c	\
#

leckinclude_HEADERS =	\
	animation.h	\
	button.h	\
	check.h		\
	hantextline.h	\
	image.h		\
	label.h		\
	list.h		\
	progressbar.h	\
	scrollbar.h	\
	slider.h	\
	textbutton.h	\
	textline.h


libleck_la_LDFLAGS = -Wl,-O1 -no-undefined			\
	-version-info $(LT_CURRENT):$(LT_REVISION):$(LT_AGE)	\
	-release $(LT_RELEASE)

libleck_la_LIBADD = \
	$(top_builddir)/lite/liblite.la	\
	@DFB_LIBS@


include $(top_srcdir)/rules/nmfile.make
