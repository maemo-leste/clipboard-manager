SUBDIRS = debian

bin_PROGRAMS = clipboard-manager

INCLUDES = 			\
	$(CM_DEPS_CFLAGS)	\
	$(X_CFLAGS)

clipboard_manager_SOURCES =	\
	clipboard-manager.c	\
	clipboard-manager.h	\
	list.c			\
	list.h			\
	manager.c		\
	gconf2xsettings.c	\
	gconf2xsettings.h	\
	xsettings-manager.c	\
	xsettings-manager.h	\
	xsettings-common.c	\
	xsettings-common.h	\
	xutils.c		\
	xutils.h

clipboard_manager_LDADD =	\
	$(CM_DEPS_LIBS)		\
	$(X_LIBS)

EXTRA_DIST = \
	clipboard.txt \
	clipboard.sh

servicedir = $(sysconfdir)/osso-af-init

service_DATA = clipboard.sh
