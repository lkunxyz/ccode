/*
 *  Adapted from HTSUtils.c in CERN httpd 3.0 (http://info.cern.ch/httpd/)
 *  by Darren Hardy <hardy@cs.colorado.edu>, November 1994.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <ctype.h>
#include <sys/types.h>
#include <time.h>
#include <sys/time.h>
#include <assert.h>

#include "http.h"

#define RFC850_STRFTIME "%A, %d-%b-%y %H:%M:%S GMT"
#define RFC1123_STRFTIME "%a, %d %b %Y %H:%M:%S GMT"
#define MIN(a, b) ((a) < (b) ? (a) : (b))

static const char *const w_space = " \t\r\n";

static int make_month(const char *s);
static int make_num(const char *s);

static const char *month_names[12] =
{
    "Jan", "Feb", "Mar", "Apr", "May", "Jun",
    "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"
};


static int
make_num(const char *s)
{
    if (*s >= '0' && *s <= '9')
	return 10 * (*s - '0') + *(s + 1) - '0';
    else
	return *(s + 1) - '0';
}

static int
make_month(const char *s)
{
    int i;
    char month[3];

    month[0] = toupper(*s);
    month[1] = tolower(*(s + 1));
    month[2] = tolower(*(s + 2));

    for (i = 0; i < 12; i++)
	if (!strncmp(month_names[i], month, 3))
	    return i;
    return -1;
}

static int
tmSaneValues(struct tm *tm)
{
    if (tm->tm_sec < 0 || tm->tm_sec > 59)
	return 0;
    if (tm->tm_min < 0 || tm->tm_min > 59)
	return 0;
    if (tm->tm_hour < 0 || tm->tm_hour > 23)
	return 0;
    if (tm->tm_mday < 1 || tm->tm_mday > 31)
	return 0;
    if (tm->tm_mon < 0 || tm->tm_mon > 11)
	return 0;
    return 1;
}

static struct tm *
parse_date_elements(const char *day, const char *month, const char *year,
	const char *time, const char *zone)
{
    static struct tm tm;
    char *t;
    memset(&tm, 0, sizeof(tm));

    if (!day || !month || !year || !time)
	return NULL;
    tm.tm_mday = atoi(day);
    tm.tm_mon = make_month(month);
    if (tm.tm_mon < 0)
	return NULL;
    tm.tm_year = atoi(year);
    if (strlen(year) == 4)
	tm.tm_year -= 1900;
    else if (tm.tm_year < 70)
	tm.tm_year += 100;
    else if (tm.tm_year > 19000)
	tm.tm_year -= 19000;
    tm.tm_hour = make_num(time);
    t = strchr(time, ':');
    if (!t)
	return NULL;
    t++;
    tm.tm_min = atoi(t);
    t = strchr(t, ':');
    if (t)
	tm.tm_sec = atoi(t + 1);
    return tmSaneValues(&tm) ? &tm : NULL;
}

#define	TIMEBUFLEN	128

/* This routine should be rewritten to not require copying the buffer - [ahc] */
static struct tm *
parse_date(const char *str, int len)
{
    struct tm *tm;
    char tmp[TIMEBUFLEN];
    char *t;
    char *wday = NULL;
    char *day = NULL;
    char *month = NULL;
    char *year = NULL;
    char *time = NULL;
    char *zone = NULL;
    int bl = MIN(len, TIMEBUFLEN - 1);

    memcpy(tmp, str, bl);
    tmp[bl] = '\0';

    for (t = (char *)strtok(tmp, ", "); t; t = (char *)strtok(NULL, ", ")) {
	if (isdigit(*t)) {
	    if (!day) {
		day = t;
		t = strchr(t, '-');
		if (t) {
		    *t++ = '\0';
		    month = t;
		    t = strchr(t, '-');
		    if (!t)
			return NULL;
		    *t++ = '\0';
		    year = t;
		}
	    } else if (strchr(t, ':'))
		time = t;
	    else if (!year)
		year = t;
	} else if (!wday)
	    wday = t;
	else if (!month)
	    month = t;
	else if (!zone)
	    zone = t;
    }
    tm = parse_date_elements(day, month, year, time, zone);
    return tm;
}

time_t parse_rfc1123(const char *str, int len)
{
    struct tm *tm;
    time_t t;
    if (NULL == str)
	return -1;
    tm = parse_date(str, len);
    if (!tm)
	return -1;
    tm->tm_isdst = -1;
#ifdef HAVE_TIMEGM
    t = timegm(tm);
#elif HAVE_TM_GMTOFF
    t = mktime(tm);
    if (t != -1) {
	struct tm *local = localtime(&t);
	t += local->tm_gmtoff;
    }
#else
    /* some systems do not have tm_gmtoff so we fake it */
    t = mktime(tm);
    if (t != -1) {
	time_t dst = 0;
#if defined (_TIMEZONE)
#elif defined (_timezone)
#elif defined(_SQUID_AIX_)
#elif defined(_SQUID_CYGWIN_)
#elif defined(_SQUID_MSWIN_)
#elif defined(_SQUID_SGI_)
#else
	extern long timezone;
#endif
	/*
	 * The following assumes a fixed DST offset of 1 hour,
	 * which is probably wrong.
	 */
	if (tm->tm_isdst > 0)
	    dst = -3600;
#if defined ( _timezone) || defined(_SQUID_WIN32_)
	t -= (_timezone + dst);
#else
	t -= (timezone + dst);
#endif
    }
#endif
    return t;
}

const char *mkrfc1123(time_t t)
{
    static char buf[128];

    struct tm *gmt = gmtime(&t);

    buf[0] = '\0';
    strftime(buf, 127, RFC1123_STRFTIME, gmt);
    return buf;
}

const char *mkhttpdlogtime(const time_t * t)
{
    static char buf[128];

    struct tm *gmt = gmtime(t);

#ifndef USE_GMT
    int gmt_min, gmt_hour, gmt_yday, day_offset;
    size_t len;
    struct tm *lt;
    int min_offset;

    /* localtime & gmtime may use the same static data */
    gmt_min = gmt->tm_min;
    gmt_hour = gmt->tm_hour;
    gmt_yday = gmt->tm_yday;

    lt = localtime(t);

    day_offset = lt->tm_yday - gmt_yday;
    /* wrap round on end of year */
    if (day_offset > 1)
	day_offset = -1;
    else if (day_offset < -1)
	day_offset = 1;

    min_offset = day_offset * 1440 + (lt->tm_hour - gmt_hour) * 60
	+ (lt->tm_min - gmt_min);

    len = strftime(buf, 127 - 5, "%d/%b/%Y:%H:%M:%S ", lt);
    snprintf(buf + len, 128 - len, "%+03d%02d",
	(min_offset / 60) % 24,
	min_offset % 60);
#else /* USE_GMT */
    buf[0] = '\0';
    strftime(buf, 127, "%d/%b/%Y:%H:%M:%S -000", gmt);
#endif /* USE_GMT */

    return buf;
}

#if 0
int
main()
{
    char *x;
    time_t t, pt;

    t = time(NULL);
    x = mkrfc1123(t);
    printf("HTTP Time: %s\n", x);

    pt = parse_rfc1123(x);
    printf("Parsed: %d vs. %d\n", pt, t);
}

#endif
