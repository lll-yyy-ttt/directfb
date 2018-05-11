INCLUDES = \
	-I$(top_srcdir)	\
	@DFB_CFLAGS@

examplesdatadir = $(DATADIR)/$(PACKAGE)/examples

AM_CPPFLAGS = \
	-DEXAMPLESDATADIR=\"$(examplesdatadir)\" 

LIBS = \
	$(top_builddir)/leck/libleck.la	\
	$(top_builddir)/lite/liblite.la	\
	@DFB_LIBS@


bin_PROGRAMS = \
	lite_bench		\
	lite_checktest		\
	lite_dfbspy		\
	lite_listtest		\
	lite_msgbox		\
	lite_progressbar	\
	lite_run		\
	lite_scrollbartest	\
	lite_simple		\
	lite_slider		\
	lite_textbuttontest	\
	lite_textlinetest	\
	lite_textlisttest

#	lite_hantextlinetest	\
#

lite_bench_SOURCES = bench.c
lite_bench_LDADD   = $(LIBS)

lite_checktest_SOURCES = checktest.c
lite_checktest_LDADD   = $(LIBS)

lite_dfbspy_SOURCES = dfbspy.c
lite_dfbspy_LDADD   = $(LIBS)

#lite_hantextlinetest_SOURCES	= hantextlinetest.c
#lite_hantextlinetest_LDADD	= $(LIBS)

lite_listtest_SOURCES = listtest.c
lite_listtest_LDADD   = $(LIBS)

lite_run_SOURCES = literun.c
lite_run_LDADD   = $(LIBS)

lite_msgbox_SOURCES = msgbox.c
lite_msgbox_LDADD   = $(LIBS)

lite_progressbar_SOURCES = progressbar.c
lite_progressbar_LDADD   = $(LIBS)

lite_scrollbartest_SOURCES = scrollbartest.c
lite_scrollbartest_LDADD   = $(LIBS)

lite_simple_SOURCES = simple.c
lite_simple_LDADD   = $(LIBS)

lite_slider_SOURCES = slider.c
lite_slider_LDADD   = $(LIBS)

lite_textbuttontest_SOURCES = textbuttontest.c
lite_textbuttontest_LDADD   = $(LIBS)

lite_textlinetest_SOURCES = textlinetest.c
lite_textlinetest_LDADD   = $(LIBS)

lite_textlisttest_SOURCES = textlisttest.c
lite_textlisttest_LDADD   = $(LIBS)


examplesdata_DATA = \
	checkbox_images.png	\
	D.png			\
	scrollbar.png		\
	stop.png		\
	stop_disabled.png	\
	stop_highlighted.png	\
	stop_pressed.png	\
	textbuttonbgnd.png	\
	toggle.png		\
	toggle_disabled.png	\
	toggle_highlighted.png	\
	toggle_pressed.png	\
	toggle_highlighted_on.png	\
	toggle_disabled_on.png	\
	progress.png		\
	progress_bg.png

EXTRA_DIST = $(examplesdata_DATA)

