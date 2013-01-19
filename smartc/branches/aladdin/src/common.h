
#ifndef _COMMON_H_
#define _COMMON_H_

#include <sys/types.h>

#include <sys/stat.h>

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <limits.h>
#include <stdint.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <assert.h>
#include <getopt.h>
#include <time.h>
#include <netinet/in.h>
#include <sys/time.h>
#include <signal.h>

/* LIBEVENT SUPPORT */
#include <event.h>

#include "list.h"
#include "array.h"
#include "string.h"

#ifdef LARGE_FILE
typedef long long aint;
#else
typedef int aint;
#endif

#endif
