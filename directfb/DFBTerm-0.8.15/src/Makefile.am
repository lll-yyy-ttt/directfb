## Makefile.am for DFBTerm/src/

INCLUDES = @DIRECTFB_CFLAGS@ @LITE_CFLAGS@

LIBS = @DIRECTFB_LIBS@ @UTIL_LIBS@ @LITE_LIBS@

AM_CPPFLAGS = -DDFBTERMSBINDIR=\"$(sbindir)/\"


bin_PROGRAMS = dfbterm

dfbterm_SOURCES = \
	dfbterm-login-support.c	\
	dfbterm-login-support.h	\
	dfbtermslist.c		\
	dfbtermslist.h		\
	lists.c			\
	lists.h			\
	subshell.c		\
	subshell.h		\
	subshell-includes.h	\
	term.c			\
	update.c		\
	vt.c			\
	vt.h			\
	vtx.h

sbin_SCRIPTS = dfbterm-pty-helper

# We really dont want $(LIBS) linked in with this
dfbterm-pty-helper: dfbterm-pty-helper.o dfbterm-login-support.o dfbterm-utmp.o
	$(CC) -o $@ $^ @UTIL_LIBS@

# Since we do dfbterm-pty-helper manually, include the relevant sources here
EXTRA_DIST = dfbterm-pty.h dfbterm-pty-helper.c dfbterm-utmp.c

install-exec-local:
        file=$(DESTDIR)$(sbindir)/dfbterm-pty-helper; \
        chown root $$file && chgrp root $$file && chmod u+s $$file

clean-local:
	rm -f dfbterm-pty-helper
