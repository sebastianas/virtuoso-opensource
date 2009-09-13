/*
 *  datesupp.c
 *
 *  $Id$
 *
 *  Date support functions
 *
 *  This file is part of the OpenLink Software Virtuoso Open-Source (VOS)
 *  project.
 *
 *  Copyright (C) 1998-2006 OpenLink Software
 *
 *  This project is free software; you can redistribute it and/or modify it
 *  under the terms of the GNU General Public License as published by the
 *  Free Software Foundation; only version 2 of the License, dated June 1991.
 *
 *  This program is distributed in the hope that it will be useful, but
 *  WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 *  General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License along
 *  with this program; if not, write to the Free Software Foundation, Inc.,
 *  51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA
 *
 */

#include "CLI.h"
#include "util/strfuns.h"
#include "datesupp.h"

/*
 *  Important preprocessor symbols for the internal ranges
 */
#define  DAY_LAST    365	/* Last day in a NON leap year */
#define  DAY_MIN     1		/* Minimum day of week/month/year */
#define  DAY_MAX     7		/* Maximum day/amount of days of week */
#define  MONTH_LAST  31		/* Highest day number in a month */
#define  MONTH_MIN   1		/* Minimum month of year */
#define  MONTH_MAX   12		/* Maximum month of year */
#define  YEAR_MIN    1		/* Minimum year able to compute */
#define  YEAR_MAX    9999	/* Maximum year able to compute */


/*
 *  The Gregorian Reformation date
 */
#define GREG_YEAR	1582
#define GREG_MONTH	10
#define GREG_FIRST_DAY	5
#define GREG_LAST_DAY	14
#define GREG_JDAYS	577737L	/* date2num (GREG_YEAR, GREG_MONTH,
				   GREG_FIRST_DAY - 1) */


/* Number of days in months */
static const int days_in_month[] =
{
  31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31
};

/* Number of past days of month */
static const int cumdays_in_month[] =
{
  0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334
};


/*
 *  Computes the number of days in February, respecting the
 *  Gregorian Reformation period
 */
int
days_in_february (const int year)
{
  int day;

  if ((year > GREG_YEAR)
      || ((year == GREG_YEAR)
	  && (GREG_MONTH == 1
	      || ((GREG_MONTH == 2)
		  && (GREG_LAST_DAY >= 28)))))
    {
      day = (year & 3) ? 28 : ((!(year % 100) && (year % 400)) ? 28 : 29);
    }
  else
    {
      day = (year & 3) ? 28 : 29;
    }

  /*
   *  Exception, the year 4 AD was historically NO leap year!
   */
  if (year == 4)
    day--;

  return day;
}


#ifdef NOT_CURRENTLY_USED
static void
dt_day_ck (int day, int month, int year, int *err, const char **err_str)
{
  if (month > 0 && month < 13)
    {
      if (month == 2 && (day < 0 || day > days_in_february (year)))
	{
	  *err_str = "Date not valid for the month and year specified";
	  *err = 1;
	}
      else if (month != 2 && (day < 0 || day > days_in_month[month - 1]))
	{
	  *err_str = "Date not valid for the month specified";
	  *err = 1;
	}
    }
  else
    {
      *err_str = "Date not valid : month not valid";
      *err = 1;
    }
}
#endif


/*
 *  Converts a given number of days of a year to a standard date
 *
 *  returns:
 *    1 in case the `day_of_year' number is valid;
 *    0 otherwise
 */
static int
yearday2date (int yday, const int is_leap_year, int *month, int *day)
{
  int i;
  int decrement_date;

  if (yday > DAY_LAST + is_leap_year || yday < DAY_MIN)
    return 0;

  decrement_date = (int) (is_leap_year && (yday > 59));
  if (decrement_date)
    yday--;
  for (i = MONTH_MIN; i < MONTH_MAX; i++)
    {
      yday -= days_in_month[i - 1];
      if (yday <= 0)
	{
	  yday += days_in_month[i - 1];
	  break;
	}
    }
  *month = i;
  *day = yday;
  if (decrement_date && *month == 2 && *day == 28)
    (*day)++;

  return 1;
}


/*
 *  Computes the absolute number of days of the given date since 0001/01/01,
 *  respecting the missing period of the Gregorian Reformation
 */
uint32
date2num (const int year, const int month, const int day)
{
  uint32 julian_days;

  julian_days = (uint32) ((year - 1) * (uint32) (DAY_LAST) + ((year - 1) >> 2));

  if (year > GREG_YEAR
      || ((year == GREG_YEAR)
	  && (month > GREG_MONTH
	      || ((month == GREG_MONTH)
		  && (day > GREG_LAST_DAY)))))
    {
      julian_days -= (uint32) (GREG_LAST_DAY - GREG_FIRST_DAY + 1);
    }
  if (year > GREG_YEAR)
    {
      julian_days += (((year - 1) / 400) - (GREG_YEAR / 400));
      julian_days -= (((year - 1) / 100) - (GREG_YEAR / 100));
      if (!(GREG_YEAR % 100) && (GREG_YEAR % 400))
	julian_days--;
    }
  julian_days += (uint32) cumdays_in_month[month - 1];
  julian_days += day;
  if (days_in_february (year) == 29 && month > 2)
    julian_days++;

  return julian_days;
}


/*
 *  Converts a delivered absolute number of days `julian_days' to
 *  a standard date (since 0001/01/01),
 *  respecting the missing period of the Gregorian Reformation
 */
void
num2date (uint32 julian_days, int *year, int *month, int *day)
{
  double x;
  int i;

  if (julian_days > GREG_JDAYS)
    julian_days += (uint32) (GREG_LAST_DAY - GREG_FIRST_DAY + 1);
  x = (double) julian_days / (DAY_LAST + 0.25);
  i = (int) x;
  if ((double) i != x)
    *year = i + 1;
  else
    {
      *year = i;
      i--;
    }
  if (julian_days > GREG_JDAYS)
    {
      /*
       *  Correction for Gregorian years
       */
      julian_days -= (uint32) ((*year / 400) - (GREG_YEAR / 400));
      julian_days += (uint32) ((*year / 100) - (GREG_YEAR / 100));
      x = (double) julian_days / (DAY_LAST + 0.25);
      i = (int) x;
      if ((double) i != x)
	*year = i + 1;
      else
	{
	  *year = i;
	  i--;
	}
      if ((*year % 400)
	  && !(*year % 100))
	julian_days--;
    }
  i = (int) (julian_days - (uint32) (i * (DAY_LAST + 0.25)));
  /*
   *  Correction for Gregorian centuries
   */
  if ((*year > GREG_YEAR)
      && (*year % 400)
      && !(*year % 100)
      && (i < ((*year / 100) - (GREG_YEAR / 100)) - ((*year / 400) - (GREG_YEAR / 400))))
    i++;
  yearday2date (i, (days_in_february (*year) == 29), month, day);
}


/*
 *  Checks whether a delivered date is valid
 */
int
ymd_valid_p (const int year, const int month, const int day)
{
  if (day < 0 ||
      month < MONTH_MIN ||
      month > MONTH_MAX ||
      year < YEAR_MIN ||
      year > YEAR_MAX ||
      (month != 2 && day > days_in_month[month - 1]) ||
      (month == 2 && day > days_in_february (year)))
    {
      return 0;
    }

  return 1;
}


/*
 *  Computes the weekday of a Gregorian/Julian calendar date
 *    (month must be 1...12) and returns 1...7 (1==mo, 2==tu...7==su).
 */
int
date2weekday (const int year, const int month, const int day)
{
  uint32 julian_days = date2num (year, month, day) % DAY_MAX;

  return ((julian_days > 2) ? (int) julian_days - 2 : (int) julian_days + 5);
}


int dt_local_tz;		/* minutes from GMT */

void
dt_now (caddr_t dt)
{
  static time_t last_time;
  static long last_frac;
  long day;
  time_t tim = time (NULL);
  struct tm tm;
#if defined(HAVE_GMTIME_R)
  struct tm result;

  tm = *(struct tm *)gmtime_r (&tim, &result);
#else
  tm = *(struct tm *)gmtime (&tim);
#endif
  day = date2num (tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday);
  DT_SET_DAY (dt, day);
  DT_SET_HOUR (dt, tm.tm_hour);
  DT_SET_MINUTE (dt, tm.tm_min);
  DT_SET_SECOND (dt, tm.tm_sec);
  if (tim == last_time)
    {
      last_frac++;
      DT_SET_FRACTION (dt, (last_frac * 1000));
    }
  else
    {
      last_frac = 0;
      last_time = tim;
      DT_SET_FRACTION (dt, 0);
    }
  DT_SET_TZ (dt, dt_local_tz);
  DT_SET_DT_TYPE (dt, DT_TYPE_DATETIME);
}


void
time_t_to_dt (time_t tim, long fraction, char *dt)
{
  long day;
#if defined(HAVE_GMTIME_R)
  struct tm result;
  struct tm tm = *(struct tm *)gmtime_r (&tim, &result);
#else
  struct tm tm = *(struct tm *)gmtime (&tim);
#endif
  day = date2num (tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday);
  DT_SET_DAY (dt, day);
  DT_SET_HOUR (dt, tm.tm_hour);
  DT_SET_MINUTE (dt, tm.tm_min);
  DT_SET_SECOND (dt, tm.tm_sec);
  DT_SET_FRACTION (dt, fraction);
  DT_SET_TZ (dt, dt_local_tz);
  DT_SET_DT_TYPE (dt, DT_TYPE_DATETIME);
}

#if defined (WIN32) && (defined (_AMD64_) || defined (_FORCE_WIN32_FILE_TIME))
int
file_mtime_to_dt (const char *name, char *dt)
{
  WIN32_FIND_DATA fdt;
  HANDLE hSearch;
  SYSTEMTIME stUTC;
  int time_set = 0;

  hSearch = FindFirstFile (name, &fdt);
  if (hSearch != INVALID_HANDLE_VALUE)
    {
      if (FileTimeToSystemTime (&fdt.ftLastWriteTime, &stUTC))
	{
	  long day;
	  day = date2num (stUTC.wYear, stUTC.wMonth, stUTC.wDay);
	  DT_SET_DAY (dt, day);
	  DT_SET_HOUR (dt, stUTC.wHour);
	  DT_SET_MINUTE (dt, stUTC.wMinute);
	  DT_SET_SECOND (dt, stUTC.wSecond);
	  DT_SET_FRACTION (dt, stUTC.wMilliseconds * 1000);
	  DT_SET_TZ (dt, dt_local_tz);
	  DT_SET_DT_TYPE (dt, DT_TYPE_DATETIME);
	  time_set = 1;
	}
      FindClose (hSearch);
    }
  return time_set;
}
#endif

#define SPERDAY (24*60*60)

void
sec2time (int sec, int *day, int *hour, int *min, int *tsec)
{
  *day = sec / SPERDAY;
  *hour = (sec - (*day * SPERDAY)) / (60 * 60);
  *min = (sec - (*day * SPERDAY) - (*hour * 60 * 60)) / 60;
  *tsec = sec % 60;
}

int
time2sec (int day, int hour, int min, int sec)
{
  return (day * SPERDAY + hour * 60 * 60 + min * 60 + sec);
}


void
ts_add (TIMESTAMP_STRUCT * ts, int n, const char *unit)
{
  int dummy;
  int32 day, sec;
  int oyear, omonth, oday, ohour, ominute, osecond;
  if (0 == n)
    return;
  day = date2num (ts->year, ts->month, ts->day);
  sec = time2sec (0, ts->hour, ts->minute, ts->second);
  if (0 == stricmp (unit, "year"))
    {
      ts->year += n;
      return;
    }
  if (0 == stricmp (unit, "month"))
    {
      int m = (ts->month - 1) + n;
      if (m >= 0)
	{
	  ts->year += m / 12;
	  ts->month = 1 + (m % 12);
	}
      else
	{
	  ts->year -= 1 - ((m + 1) / 12);
	  ts->month = 12 + ((m + 1) % 12);
	}
      return;
    }

  if (0 == stricmp (unit, "second"))
    sec += n;
  else if (0 == stricmp (unit, "minute"))
    sec += 60 * n;
  else if (0 == stricmp (unit, "hour"))
    sec += 60 * 60 * n;
  else if (0 == stricmp (unit, "day"))
    day += n;

  if (sec < 0)
    {
      day = day - (1 + ((-sec) / SPERDAY));

      sec = sec % SPERDAY;

      if (sec == 0)
	day++;

      sec = SPERDAY + sec;
    }
  else
    {
      day = day + sec / SPERDAY;
      sec = sec % SPERDAY;
    }
  num2date (day, &oyear, &omonth, &oday);
  sec2time (sec, &dummy, &ohour, &ominute, &osecond);
  ts->year = oyear;
  ts->month = omonth;
  ts->day = oday;
  ts->hour = ohour;
  ts->minute = ominute;
  ts->second = osecond;
}


void
dt_to_GMTimestamp_struct (ccaddr_t dt, GMTIMESTAMP_STRUCT * ts)
{
  int year, month, day;
  num2date (DT_DAY (dt), &year, &month, &day);
  ts->year = year;
  ts->month = month;
  ts->day = day;
  ts->hour = DT_HOUR (dt);
  ts->minute = DT_MINUTE (dt);
  ts->second = DT_SECOND (dt);
  ts->fraction = DT_FRACTION (dt);
}

void
dt_to_timestamp_struct (ccaddr_t dt, TIMESTAMP_STRUCT * ts)
{
  dt_to_GMTimestamp_struct (dt, ts);
  ts_add (ts, DT_TZ (dt), "minute");
}

int dt_validate (caddr_t dt)
{
  if (!IS_BOX_POINTER(dt))
    return 1;
  if (DT_LENGTH != box_length (dt))
    return 1;
  if ((23 < (unsigned)(DT_HOUR(dt))) || (59 < (unsigned)(DT_MINUTE(dt))) || (60 < (unsigned)(DT_SECOND(dt))))
    return 1;
  return 0;
}

void
GMTimestamp_struct_to_dt (GMTIMESTAMP_STRUCT * ts, char *dt)
{
  uint32 day;
  day = date2num (ts->year, ts->month, ts->day);
  DT_SET_DAY (dt, day);
  DT_SET_HOUR (dt, ts->hour);
  DT_SET_MINUTE (dt, ts->minute);
  DT_SET_SECOND (dt, ts->second);
  DT_SET_FRACTION (dt, ts->fraction);
  DT_SET_TZ (dt, 0);
  DT_SET_DT_TYPE (dt, DT_TYPE_DATETIME);
}

void
timestamp_struct_to_dt (TIMESTAMP_STRUCT * ts_in, char *dt)
{
  TIMESTAMP_STRUCT ts_tmp;
  TIMESTAMP_STRUCT *ts = &ts_tmp;
  ts_tmp = *ts_in;
  ts_add (ts, -dt_local_tz, "minute");
  GMTimestamp_struct_to_dt (ts, dt);
  DT_SET_TZ (dt, dt_local_tz);
}

void
dt_to_date_struct (char *dt, DATE_STRUCT * ots)
{
  TIMESTAMP_STRUCT ts;
  dt_to_timestamp_struct (dt, &ts);
  ots->year = ts.year;
  ots->month = ts.month;
  ots->day = ts.day;
}


void
date_struct_to_dt (DATE_STRUCT * ds, char *dt)
{
  TIMESTAMP_STRUCT ts;

  memset (&ts, 0, sizeof (ts));
  ts.year = ds->year;
  ts.month = ds->month;
  ts.day = ds->day;

  timestamp_struct_to_dt (&ts, dt);
  DT_SET_DT_TYPE (dt, DT_TYPE_DATE);
}


void
dt_to_time_struct (char *dt, TIME_STRUCT * ots)
{
  TIMESTAMP_STRUCT ts;
  dt_to_timestamp_struct (dt, &ts);
  ots->hour = ts.hour;
  ots->minute = ts.minute;
  ots->second = ts.second;
}


void
time_struct_to_dt (TIME_STRUCT * ts, char *dt)
{
   TIMESTAMP_STRUCT tss;
   memset (&tss, 0, sizeof (tss));
   tss.hour = ts->hour;
   tss.minute = ts->minute;
   tss.second = ts->second;
   timestamp_struct_to_dt (&tss, dt);
   DT_SET_DT_TYPE (dt, DT_TYPE_TIME);
}


void
dt_date_round (char *dt)
{
  TIMESTAMP_STRUCT ts;
  dt_to_timestamp_struct (dt, &ts);
  ts.hour = 0;
  ts.minute = 0;
  ts.second = 0;
  ts.fraction = 0;
  timestamp_struct_to_dt (&ts, dt);
  DT_SET_DT_TYPE (dt, DT_TYPE_DATE);
}

int isdts_mode = 1;

void
dt_init ()
{
  time_t lt, gt;
  struct tm ltm;
  struct tm gtm;
  time_t tim;
#if defined(HAVE_GMTIME_R)
  struct tm result;
#endif

  tim = time (NULL);
  ltm = *localtime (&tim);
#if defined(HAVE_GMTIME_R)
  gtm = *(struct tm *)gmtime_r (&tim, &result);
#else
  gtm = *(struct tm *)gmtime (&tim);
#endif
  lt = mktime (&ltm);
  gt = mktime (&gtm);
  dt_local_tz = (int) (lt - gt) / 60;
  if (ltm.tm_isdst && isdts_mode)  /* Check daylight saving */
    dt_local_tz = dt_local_tz + 60;
}

long
dt_fraction_part_ck (char *str, long factor, int *err)
{
  long acc = 0;
  if (NULL == str)
    return 0;
  if (!isdigit (str[0]))
    {
      *err |= 1;
      return 0;
    }
  do
    {
      if (factor)
        acc = acc * 10 + (str[0] - '0');
      str++;
      factor /= 10;
    } while (isdigit (str[0]));
  return acc * (factor ? factor : 1);
}

int
dt_part_ck (char *str, int min, int max, int *err)
{
  int n;
  if (!str)
    n = 0;
  else
    {
      if (1 != sscanf (str, "%d", &n))
	{
	  *err |= 1;
	  return 0;
	}
    }
  if (n < min || n > max)
    {
      *err |= 1;
      return 0;
    }
  return n;
}

void
dt_to_string (const char *dt, char *str, int len)
{
  TIMESTAMP_STRUCT ts;
  int dt_type, len_before_fra;
  char *tail = str;
  dt_to_timestamp_struct (dt, &ts);
  dt_type = DT_DT_TYPE (dt);
  len_before_fra = len - (ts.fraction ? 10 : 0);
  switch (dt_type)
    {
      case DT_TYPE_DATE:
        snprintf (str, len, "%04d-%02d-%02d",
          ts.year, ts.month, ts.day);
        return;
      case DT_TYPE_TIME:		/*  012345678 */
        if (len_before_fra < 8)		/* "hh:mm:ss" */
          goto short_buf; /* see below */
        tail += snprintf (str, len_before_fra, "%02d:%02d:%02d",
          ts.hour, ts.minute, ts.second );
	break;
      default:				/*  01234567890123456789 */
        if (len_before_fra < 19)	/* "yyyy-mm-dd hh:mm:ss" */
          goto short_buf; /* see below */
	tail += snprintf (str, len_before_fra, "%04d-%02d-%02d %02d:%02d:%02d",
	  ts.year, ts.month, ts.day, ts.hour, ts.minute, ts.second );
	break;
    }
  if (ts.fraction)
    {
      if (ts.fraction % 1000)
	tail += snprintf (tail, (str + len) - tail, ".%09d", (int)ts.fraction);
      else if (ts.fraction % 1000000)
	tail += snprintf (tail, (str + len) - tail, ".%06d", (int)(ts.fraction / 1000));
      else
	tail += snprintf (tail, (str + len) - tail, ".%03d", (int)(ts.fraction / 1000000));
    }
  return;

short_buf:
  snprintf (str, len, "??? short output buffer for dt_to_string()");
}

void
dt_to_iso8601_string (const char *dt, char *str, int len)
{
  GMTIMESTAMP_STRUCT ts;
  int tz = DT_TZ (dt);
  int dt_type, len_before_tz, len_before_fra;
  char *tail = str;
  dt_to_timestamp_struct (dt, &ts);
  dt_type = DT_DT_TYPE (dt);
  len_before_tz = len - (tz ? 6 : 1);
  len_before_fra = len_before_tz - (ts.fraction ? 10 : 0);
  switch (dt_type)
    {
      case DT_TYPE_DATE:
        snprintf (str, len, "%04d-%02d-%02d",
          ts.year, ts.month, ts.day);
        return;
      case DT_TYPE_TIME:		/*  012345678 */
        if (len_before_fra < 8)		/* "hh:mm:ss" */
          goto short_buf; /* see below */
        tail += snprintf (str, len_before_fra, "%02d:%02d:%02d",
          ts.hour, ts.minute, ts.second );
	break;
      default:				/* 01234567890123456789 */
        if (len_before_fra < 19)	/* yyyy-mm-ddThh:mm:ss */
          goto short_buf; /* see below */
	tail += snprintf (str, len_before_fra, "%04d-%02d-%02dT%02d:%02d:%02d",
	  ts.year, ts.month, ts.day, ts.hour, ts.minute, ts.second );
	break;
    }
  if (ts.fraction)
    {
      if (ts.fraction % 1000)
	tail += snprintf (tail, (str + len) - tail, ".%09d", (int)ts.fraction);
      else if (ts.fraction % 1000000)
	tail += snprintf (tail, (str + len) - tail, ".%06d", (int)(ts.fraction / 1000));
      else
	tail += snprintf (tail, (str + len) - tail, ".%03d", (int)(ts.fraction / 1000000));
    }
  if (tz)
    snprintf (tail, (str + len) - tail, "%+03d:%02d", tz / 60, abs (tz) % 60);
  else if (((str + len) - tail) > 2)
    strcpy (tail, "Z");
  return;

short_buf:
  snprintf (str, len, "??? short output buffer for dt_to_iso8601_string()");
}

void
dt_to_rfc1123_string (const char *dt, char *str, int len)
{
  GMTIMESTAMP_STRUCT ts;
  char * wkday [] = {"Mon", "Tue", "Wed", "Thu", "Fri", "Sat", "Sun"};
  char * monday [] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};
  dt_to_GMTimestamp_struct (dt, &ts);
  /* Mon, 01 Feb 2000 00:00:00 GMT */
  snprintf (str, len, "%s, %02d %s %04d %02d:%02d:%02d GMT",
	   wkday [date2weekday (ts.year, ts.month, ts.day) - 1], ts.day, monday [ts.month - 1], ts.year, ts.hour, ts.minute, ts.second);
	   /*ts.year, ts.month, ts.day, ts.hour, ts.minute, ts.second, (long) ts.fraction);*/
}

void
dt_to_ms_string (const char *dt, char *str, int len)
{
  TIMESTAMP_STRUCT ts;
  char * monday [] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};
  dt_to_timestamp_struct (dt, &ts);

  /* 01-Feb-2000 00:00:00 */
  snprintf (str, len, "%02d-%s-%04d %02d:%02d:%02d",
	   ts.day, monday [ts.month - 1], ts.year, ts.hour, ts.minute, ts.second);
}

#ifdef DT_DEBUG
extern void
iso8601_or_odbc_string_to_dt_impl (const char *str, char *dt, int dtflags, int dt_type, caddr_t *err_msg_ret);

void
iso8601_or_odbc_string_to_dt_1 (const char *str, char *dt, int dtflags, int dt_type, caddr_t *err_msg_ret)
{
  FILE *f = fopen ("iso8601_or_odbc_string_to_dt.log", "at");
  iso8601_or_odbc_string_to_dt_impl (str, dt, dtflags, dt_type, err_msg_ret);
  if (NULL != f)
    {
      if (NULL != err_msg_ret[0])
        {
          fprintf (f, "%s\n\t%s\n", str, err_msg_ret[0]);
        }
      else
        {
          char tmp[100];
          dt_to_iso8601_string (dt, tmp, sizeof (tmp));
          fprintf (f, "%s\t--> %s\n", str, tmp);
        }
      fclose (f);
    }
}

void
iso8601_or_odbc_string_to_dt_impl (const char *str, char *dt, int dtflags, int dt_type, caddr_t *err_msg_ret)
#else
void
iso8601_or_odbc_string_to_dt_1 (const char *str, char *dt, int dtflags, int dt_type, caddr_t *err_msg_ret)
#endif
{
  int tzsign = 0, res_flags = 0, tzmin = dt_local_tz;
  int odbc_braces = 0, new_dtflags, new_dt_type = 0;
  int us_mdy_format = 0;
  const char *tail, *group_end;
  int fld_values[9];
  static int fld_min_values[9] =	{ 1	, 1	, 1	, 0	, 0	, 0	, 0		, 0	, 0	};
  static int fld_max_values[9] =	{ 9999	, 12	, 31	, 23	, 59	, 61	, 999999999	, 14	, 59	};
  static int fld_max_lengths[9] =	{ 4	, 2	, 2	, 2	, 2	, 2	, -1		, 2	, 2	};
  static int delms[9] =			{ '-'	, '-'	, 'T'	, ':'	, ':'	, '\0', '\0'		, ':'	, '\0'	};
  static const char *names[9] =		{"year"	,"month","day"	,"hour"	,"minute","second","fraction","TZ hour","TZ minute"};
  static int days_in_months[12] = { 31, -1, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };
  int fld_idx;
  tail = group_end = str;
  memcpy (fld_values, fld_min_values, 9 * sizeof (int));
  if ((DTFLAG_ALLOW_ODBC_SYNTAX & dtflags) && ('{' == tail[0]))
    {
      odbc_braces = 1;
      if (('t' == tail[1]) && ('s' == tail[2]))
        {
          tail += 3;
          new_dt_type = DT_TYPE_DATETIME;
          new_dtflags = DTFLAG_DATE | DTFLAG_TIME | DTFLAG_TIMEZONE;
        }
      else if ('d' == tail[1])
        {
          tail += 2;
          new_dt_type = DT_TYPE_DATE;
          new_dtflags = DTFLAG_DATE;
        }
      else if ('t' == tail[1])
        {
          tail += 2;
          new_dt_type = DT_TYPE_TIME;
          new_dtflags = DTFLAG_TIME | DTFLAG_TIMEZONE;
        }
      else
        {
          err_msg_ret[0] = box_dv_short_string ("Invalid ODBC literal type after '{', only 'd', 't', and 'ts' are supported");
          return;
        }
      if (DTFLAG_FORMAT_SETS_FLAGS & dtflags)
        {
          dtflags = (dtflags & ~(DTFLAG_DATE | DTFLAG_TIME | DTFLAG_TIMEZONE)) | new_dtflags;
          if (-1 != dt_type)
            dt_type = new_dt_type;
        }
      else if ((dtflags & (DTFLAG_DATE | DTFLAG_TIME)) != (new_dtflags & (DTFLAG_DATE | DTFLAG_TIME)))
        {
          err_msg_ret[0] = box_dv_short_string ("ODBC literal type does not match the expected one");
          return;
        }
      while (' ' == tail[0]) tail++;
      if ('\'' != tail[0])
        {
          err_msg_ret[0] = box_dv_short_string ("Syntax error in ODBC literal (single-quoted constant expected after literal type");
          return;
        }
    }
  for (fld_idx = 0; fld_idx < 9; fld_idx++)
    {
      int fld_flag = (1 << fld_idx);
      int fldlen, fld_maxlen, fld_value;
      int expected_delimiter;
      if ('\0' == tail[0])
        break;
      if ((DTFLAG_ALLOW_ODBC_SYNTAX & dtflags) && ('\'' == tail[0]))
        {
          tail++;
          while (' ' == tail[0]) tail++;
          if ('}' != tail[0])
            {
              err_msg_ret[0] = box_dv_short_string ("Syntax error in ODBC literal (missing '}' after closing quote)");
              return;
            }
          tail++;
          break;
        }
      if (0 == (dtflags & fld_flag))
        continue;
      if ((DTFLAG_YY == fld_flag) && !(DTFLAG_ALLOW_ODBC_SYNTAX & dtflags))
        while ('0' == tail[0]) tail++;
      if (DTFLAG_ZH == fld_flag)
        {
          if ('-' == tail[-1])
            tzsign = 1;
        }
      for (group_end = tail; isdigit (group_end[0]); group_end++) /*no body*/;
      fldlen = group_end - tail;
      fld_maxlen = fld_max_lengths[fld_idx];
      if (('/' == group_end[0]) || (('.' == group_end[0]) && (DTFLAG_MM >= fld_flag)))
        {
          if (!(DTFLAG_ALLOW_ODBC_SYNTAX & dtflags))
            {
              err_msg_ret[0] = box_dv_short_string ("mm/dd/yyyy format is not allowed, needs yyyy-mm-dd");
              return;
            }
          if (DTFLAG_YY == fld_flag)
            us_mdy_format = group_end[0];
          else if ((us_mdy_format != group_end[0]) || (DTFLAG_MM != fld_flag))
            {
              err_msg_ret[0] = box_sprintf (50, "Syntax error in ODBC literal (misplaced '%c')", group_end[0]);
              return;
            }
        }
/*Check for field length and parse special cases like missing delimiters in year-month-day or hour:minute */
      if (fldlen == fld_maxlen)
        goto field_length_checked; /* see below */
      if (DTFLAG_YY == fld_flag)
        {
          if (fldlen < fld_maxlen)
            goto field_length_checked; /* see below */
          if (8 == fldlen)
            {
              fld_values[0] = ((((((tail[0]-'0') * 10) + (tail[1]-'0')) * 10) + (tail[2]-'0')) * 10) + (tail[3]-'0');
              fld_values[1] = ((tail[4]-'0') * 10) + (tail[5]-'0');
              tail += 6;
              fld_idx++;
              res_flags |= DTFLAG_YY | DTFLAG_MM;
              continue;
            }
        }
      if (DTFLAG_ALLOW_ODBC_SYNTAX & dtflags)
        {
          if ((DTFLAG_MM <= fld_flag) && (DTFLAG_SS >= fld_flag) && (fldlen > 0) && (fldlen <= 2))
            goto field_length_checked; /* see below */
          if ((us_mdy_format) && (DTFLAG_DD == fld_flag) && (fldlen > 0) && (fldlen <= 4))
            goto field_length_checked; /* see below */
        }
      if ((DTFLAG_SF == fld_flag) && (fldlen > 0))
        goto field_length_checked; /* see below */
      if ((DTFLAG_HH == fld_flag) && (4 == fldlen) && (tail > str) && (('T' == tail[-1]) || (('X' == tail[-1]) && ('T' == tail[-2]))))
        {
          fld_values[3] = ((tail[0]-'0') * 10) + (tail[1]-'0');
          fld_values[4] = ((tail[2]-'0') * 10) + (tail[3]-'0');
          fld_values[5] = 0;
          fld_idx += 2;
          tail += 4;
          res_flags |= DTFLAG_HH | DTFLAG_MIN | DTFLAG_SS;
          if (('Z' == group_end[0]) && ('\0' == group_end[1]))
            {
              tzmin = 0;
              group_end++;
              break;
            }
          continue;
        }
      err_msg_ret[0] = box_sprintf (500, "Incorrect %s field length", names[fld_idx]);
      return;

field_length_checked:
      if (DTFLAG_SF == fld_flag)
        {
          int mult = 1000000000;
          int cctr;
          fld_value = 0;
          for (cctr = 0; ((cctr < 9) && (cctr < fldlen)); cctr++)
            {
              mult /= 10;
              fld_value += (tail[cctr] - '0') * mult;
            }
        }
      else
        fld_value = atoi (tail);
      fld_values[fld_idx] = fld_value;
      res_flags |= fld_flag;

      expected_delimiter = delms[fld_idx];
      if (expected_delimiter == group_end[0])
        goto field_delim_checked; /* see below */
      if ('\0' == group_end[0])
        goto field_delim_checked; /* see below */
      if (NULL != strchr ("+-Z",group_end[0]))
        {
          tzmin = 0; /* Default timezone is dropped because an explicit one is in place */
          if ('Z' == group_end[0])
            {
              if ('\0' != group_end[1])
                {
                  err_msg_ret[0] = box_dv_short_string ("Invalid timezone (extra characters after 'Z')");
                  return;
                }
            }
          if (DTFLAG_SS == fld_flag)
            fld_idx++;
          else if ((DTFLAG_DD == fld_flag) && (!(dtflags & (DTFLAG_HH | DTFLAG_MIN | DTFLAG_SS | DTFLAG_SF))))
            fld_idx += 4;
          goto field_delim_checked; /* see below */
        }
      if ((DTFLAG_SS == fld_flag) && ('.' == group_end[0]))
        goto field_delim_checked; /* see below */
      if ((DTFLAG_DD == fld_flag) && (' ' == group_end[0]) && (DTFLAG_ALLOW_ODBC_SYNTAX & dtflags))
        goto field_delim_checked; /* see below */
      if (us_mdy_format)
        {
          if ((DTFLAG_MM >= fld_flag) && (us_mdy_format == group_end[0]))
            goto field_delim_checked; /* see below */
          if ((DTFLAG_MIN == fld_flag) && ('.' == group_end[0]))
            goto field_delim_checked; /* see below */
          if ((DTFLAG_SS == fld_flag) && (' ' == group_end[0]))
            goto field_delim_checked; /* see below */
        }
      err_msg_ret[0] = box_sprintf (500, "Incorrect %s delimiter", names[fld_idx]);
      return;

field_delim_checked:

      if (DTFLAG_ZH == fld_flag)
        tzmin = 0;
      tail = group_end;
      if ('\0' == tail[0])
        continue;
      if (('T' == tail[0]) && ('X' == tail[1]))
        tail++;
      tail++;
    }
  if ('\0' != tail[0])
    {
      err_msg_ret[0] = box_sprintf (500, "Extra symbols (%.200s) after the end of data", group_end);
      return;
    }
  if (us_mdy_format)
    { /* Dig fields in mm/dd/yy order... */
      int m = fld_values[0];
      int d = fld_values[1];
      int y = fld_values[2];
      if ((m <= 12) && (y >= 1000))
        {
          fld_values[0] = y; fld_values[1] = m; fld_values[2] = d; /* ... and dig in ISO one */
        }
    }
  for (fld_idx = 0; fld_idx < 9; fld_idx++)
    {
      int fld_flag = (1 << fld_idx);
      int fld_value = fld_values[fld_idx];
      if (0 == (res_flags & fld_flag))
        continue; /* not set -- no check */
      if ((fld_value < fld_min_values[fld_idx]) || (fld_value > fld_max_values[fld_idx]))
        {
          err_msg_ret[0] = box_sprintf (500, "Incorrect %s value", names[fld_idx]);
          return;
        }
      if (DTFLAG_DD == fld_flag)
        {
          int month = fld_values[1];
          int days_in_this_month = days_in_months[month-1];
	  if (2 == month) /* February */
	    days_in_this_month = days_in_february (fld_values[0]);
	  if (fld_value > days_in_this_month)
            {
              err_msg_ret[0] = box_sprintf (500, "Too many days (%d, the month has only %d)", fld_value, days_in_this_month);
              return;
            }
        }
    }
  tzmin += (60 * fld_values[7]) + fld_values[8];
  if (tzsign)
    tzmin *= -1;
  dt_from_parts (dt,
    fld_values[0], fld_values[1], fld_values[2],
    fld_values[3], fld_values[4], fld_values[5],
    fld_values[6], tzmin );
  if ((DT_TYPE_TIME == dt_type) || (DTFLAG_FORCE_DAY_ZERO & dtflags))
    DT_SET_DAY (dt, DAY_ZERO);
  if (0 <= dt_type)
    DT_SET_DT_TYPE (dt, dt_type);
  err_msg_ret[0] = NULL;
  return;
}

void
iso8601_or_odbc_string_to_dt (const char *str, char *dt, int dtflags, int dt_type, caddr_t *err_msg_ret)
{
  caddr_t copy = box_string (str);
  char *start, *end;
  start = copy;
  end = copy + box_length (copy) - 2;
  while (isspace (*start))
   start++;
  while (end && end >= start && isspace (*end))
    *(end--) = 0;
  iso8601_or_odbc_string_to_dt_1 (start, dt, dtflags, dt_type, err_msg_ret);
  dk_free_box (copy);
}

int
http_date_to_dt (const char *http_date, char *dt)
{
  char month[4] /*, weekday[10] */;
  unsigned day, year, hour, minute, second;
  int idx, fmt, month_number;
  GMTIMESTAMP_STRUCT ts_tmp, *ts = &ts_tmp;
  const char *http_end_of_weekday = http_date;

  day = year = hour = minute = second = 0;
  month[0] /* = weekday[0] = weekday[9] */ = 0;
  memset (ts, 0, sizeof (TIMESTAMP_STRUCT));

  for (idx = 0; isalpha (http_end_of_weekday[0]) && (idx < 9); idx++)
    http_end_of_weekday++;
  /*weekday[idx] = '\0';*/

  /* rfc 1123 */
  if (6 == sscanf (http_end_of_weekday, ", %2u %3s %4u %2u:%2u:%u GMT",
	&day, &(month[0]), &year, &hour, &minute, &second) &&
    (3 == (http_end_of_weekday - http_date)) )
    fmt = 1123;
  /* rfc 850 */
  else if (6 == sscanf (http_end_of_weekday, ", %2u-%3s-%2u %2u:%2u:%u GMT",
	&day, &(month[0]), &year, &hour, &minute, &second) &&
    (6 <= (http_end_of_weekday - http_date)) )
    {
      if (year > 0 && year < 100)
	year = year + 1900;
      fmt = 850;
    }
  /* asctime */
  else if (6 == sscanf (http_end_of_weekday, " %3s %2u %2u:%2u:%u %4u",
	 month, &day, &hour, &minute, &second, &year) &&
    (3 == (http_end_of_weekday - http_date)) )
    fmt = -1;
  else
    return 0;

  if (day > 31 || hour > 24 || minute > 60 || second > 60)
    return 0;

  if (!strncmp (month, "Jan", 3))
    month_number = 1;
  else if (!strncmp (month, "Feb", 3))
    month_number = 2;
  else if (!strncmp (month, "Mar", 3))
    month_number = 3;
  else if (!strncmp (month, "Apr", 3))
    month_number = 4;
  else if (!strncmp (month, "May", 3))
    month_number = 5;
  else if (!strncmp (month, "Jun", 3))
    month_number = 6;
  else if (!strncmp (month, "Jul", 3))
    month_number = 7;
  else if (!strncmp (month, "Aug", 3))
    month_number = 8;
  else if (!strncmp (month, "Sep", 3))
    month_number = 9;
  else if (!strncmp (month, "Oct", 3))
    month_number = 10;
  else if (!strncmp (month, "Nov", 3))
    month_number = 11;
  else if (!strncmp (month, "Dec", 3))
    month_number = 12;
  else
    return 0;

  ts->year = year;
  ts->month = month_number;
  ts->day = day;

  ts->hour = hour;
  ts->minute = minute;
  ts->second = second;
#if 0 /* This 'if' is NOT valid. Citation from RFC 2616:
All HTTP date/time stamps MUST be represented in Greenwich Mean Time (GMT), without exception.
For the purposes of HTTP, GMT is exactly equal to UTC (Coordinated Universal Time).
This is indicated in the first two formats by the inclusion of "GMT" as the three-letter abbreviation for time zone,
and MUST be assumed when reading the asctime format. */
  if (1123 == fmt || 850 == fmt) /* these formats are explicitly for GMT */
#endif
  GMTimestamp_struct_to_dt (ts, dt);
  return 1;
}


void
dt_to_tv (char *dt, char *dv)
{
  time_t tt;
  long ttf;
  struct tm tm;
  TIMESTAMP_STRUCT ts;
  memset (&tm, 0, sizeof (tm));
  dt_to_timestamp_struct (dt, &ts);
  tm.tm_year = ts.year - 1900;
  tm.tm_mon = ts.month - 1;
  tm.tm_mday = ts.day;
  tm.tm_hour = ts.hour;
  tm.tm_min = ts.minute;
  tm.tm_sec = ts.second;
  tm.tm_isdst = -1;
  tt = mktime (&tm);
  ttf = DT_FRACTION (dt);
  ((uint32 *) dv)[0] = LONG_TO_EXT (tt);
  ((uint32 *) dv)[1] = LONG_TO_EXT (ttf);
}

void
dt_to_parts (char *dt, int *year, int *month, int *day, int *hour, int *minute, int *second, int *fraction)
{
  TIMESTAMP_STRUCT ts;
  dt_to_timestamp_struct (dt, &ts);
  if (year)
    *year = ts.year;
  if (month)
    *month = ts.month;
  if (day)
    *day = ts.day;
  if (hour)
    *hour = ts.hour;
  if (minute)
    *minute = ts.minute;
  if (second)
    *second = ts.second;
  if (fraction)
    *fraction = ts.fraction;
}

void
dt_from_parts (char *dt, int year, int month, int day, int hour, int minute, int second, int fraction, int tz)
{
  GMTIMESTAMP_STRUCT ts;
  ts.year = year;
  ts.month = month;
  ts.day = day;
  ts.hour = hour;
  ts.minute = minute;
  ts.second = second;
  ts.fraction = fraction;
  ts_add (&ts, -tz, "minute");
  GMTimestamp_struct_to_dt (&ts, dt);
  DT_SET_TZ (dt, tz);
}

void
dt_make_day_zero (char *dt)
{
  GMTIMESTAMP_STRUCT tss;
  dt_to_timestamp_struct (dt, &tss); /*??? not sure this is correct */
  GMTimestamp_struct_to_dt (&tss, dt);
  DT_SET_DAY (dt, DAY_ZERO);
  DT_SET_DT_TYPE (dt, DT_TYPE_TIME);
}

#ifdef DEBUG
void
dt_audit_fields (char *dt)
{
  int arg_dt_type = DT_DT_TYPE (dt);
  int d = DT_DAY(dt);
  int h = DT_HOUR(dt);
  int m,s,f;
  if (0 == d) GPF_T1 ("Zero day in dt_audit_fields()");
  if (h >= 24) GPF_T1 ("bad hour in DT_TYPE_DATE dt_audit_fields()");
  switch (arg_dt_type)
    {
    case DT_TYPE_DATETIME:
      if (DAY_ZERO == d) GPF_T1 ("DAY_ZERO in DT_TYPE_DATETIME dt_audit_fields()");
      break;
    case DT_TYPE_DATE:
      if (DAY_ZERO == d) GPF_T1 ("DAY_ZERO in DT_TYPE_DATE dt_audit_fields()");
      m = DT_MINUTE(dt);
      s = DT_SECOND(dt);
      f = DT_FRACTION(dt);
      if (m % 15) GPF_T1 ("Bad timezone diff applied to minutes in DT_TYPE_DATE dt_audit_fields()");
      if (0 != s) GPF_T1 ("nonzero second in DT_TYPE_DATE dt_audit_fields()");
      if (0 != f) GPF_T1 ("nonzero fraction in DT_TYPE_DATE dt_audit_fields()");
      break;
    case DT_TYPE_TIME:
      if (DAY_ZERO != d) GPF_T1 ("DAY_ZERO in DT_TYPE_TIME dt_audit_fields()");
      break;
    default:
        GPF_T1 ("Wrong DT_DT_TYPE in dt_audit_fields()");
  }
}
#endif

