#ifndef DFBTERM_PTY_H
#define DFBTERM_PTY_H

typedef enum {
	DFBTERM_PTY_OPEN_PTY_UTMP = 1,
	DFBTERM_PTY_OPEN_PTY_UWTMP,
	DFBTERM_PTY_OPEN_PTY_WTMP,
	DFBTERM_PTY_OPEN_PTY_LASTLOG,
	DFBTERM_PTY_OPEN_PTY_LASTLOGUTMP,
	DFBTERM_PTY_OPEN_PTY_LASTLOGUWTMP,
	DFBTERM_PTY_OPEN_PTY_LASTLOGWTMP,
	DFBTERM_PTY_OPEN_NO_DB_UPDATE,
	DFBTERM_PTY_RESET_TO_DEFAULTS,
	DFBTERM_PTY_CLOSE_PTY
} GnomePtyOps;

void *update_dbs         (int utmp, int wtmp, int lastlog, char *login_name, char *display_name, char *term_name);
void *write_login_record (char *login_name, char *display_name, char *term_name, int utmp, int wtmp, int lastlog);
void write_logout_record (void *data, int utmp, int wtmp);

#endif
