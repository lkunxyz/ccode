sbin_PROGRAMS=smartc

LUA_INCLUDE=$(top_builddir)/include/lua
DEFAULT_PREFIX=$(prefix)
DEFAULT_CONF_FILE=$(prefix)/etc/smartc.conf
DEFAULT_ACCESS_LOG=$(prefix)/access.log
DEFAULT_DEBUG_LOG=$(prefix)/debug.log
DEFAULT_WARN_LOG=$(prefix)/warning.log
DEFAULT_ERROR_LOG=$(prefix)/error.log
SMARTC_GLIB=-lglib-2.0 -lgthread-2.0
SMARTC_GLIB_CFLAGS=-I/usr/include/glib-2.0 -I/usr/lib/glib-2.0/include
AM_CFLAGS=$(SMARTC_GLIB_CFLAGS)
if USE_POSIX_AIO
AIO_SOURCE=file_posix_aio.c
endif
if USE_LINUX_AIO
AIO_SOURCE=file_linux_aio.c
endif
if USE_POSIX_AIO
AIO_LIB=-lpthread \
		-lrt
else 
AIO_LIB=-laio
endif
DEFS+=-D_GNU_SOURCE -DUSE_LUA=1\
      -DDEFAULT_CONF_FILE=\"$(DEFAULT_CONF_FILE)\" \
	  -DDEFAULT_ACCESS_LOG=\"$(DEFAULT_ACCESS_LOG)\" \
	  -DDEFAULT_DEBUG_LOG=\"$(DEFAULT_DEBUG_LOG)\" \
	  -DDEFAULT_WARN_LOG=\"$(DEFAULT_WARN_LOG)\" \
	  -DDEFAULT_ERROR_LOG=\"$(DEFAULT_ERROR_LOG)\"

smartc_SOURCES=config.h server.h \
				   server.c \
				   http.c \
				   http.h \
				   http_header.h \
				   http_header.c \
				   string.c \
				   string.h \
				   array.h \
				   array.c \
				   hashtable.c \
				   hashtable.h \
				   cache_file.c \
				   cache_file.h \
				   log.h \
				   log.c \
				   sock_io.c \
				   sock_io.h \
				   sock_epoll.c \
				   sock_epoll.h \
				   md5.h \
				   md5.c \
				   rfc1123.c \
				   list.h \
				   fd_event.h \
				   fd_event.c \
				   file_aio.h \
				   accesslog.h \
				   accesslog.c \
				   cache_index.h \
				   cache_index.c \
				   cache_dir.h \
				   cache_dir.c \
				   conf.h \
				   conf.c \
				   $(AIO_SOURCE) \
				   cache_control.h \
				   cache_control.c \
				   lua_api.h \
				   lua_api.c

smartc_LDADD=-lssl -lm\
			 $(AIO_LIB) \
			 $(SMARTC_GLIB) \
			 -llua5.1

sysconf_DATA=smartc.conf.default
