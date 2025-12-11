// -----------------------------------------------------------------------
//  Copyright (C) 2006 - 2007 FRESCOR consortium partners:
//
//    Universidad de Cantabria,              SPAIN
//    University of York,                    UK
//    Scuola Superiore Sant'Anna,            ITALY
//    Kaiserslautern University,             GERMANY
//    Univ. Politécnica  Valencia,           SPAIN
//    Czech Technical University in Prague,  CZECH REPUBLIC
//    ENEA                                   SWEDEN
//    Thales Communication S.A.              FRANCE
//    Visual Tools S.A.                      SPAIN
//    Rapita Systems Ltd                     UK
//    Evidence                               ITALY
//
//    See http://www.frescor.org for a link to partners' websites
//
//           FRESCOR project (FP6/2005/IST/5-034026) is funded
//        in part by the European Union Sixth Framework Programme
//        The European Union is not liable of any use that may be
//        made of this code.
//
//
//  based on previous work (FSF) done in the FIRST project
//
//   Copyright (C) 2005  Mälardalen University, SWEDEN
//                       Scuola Superiore S.Anna, ITALY
//                       Universidad de Cantabria, SPAIN
//                       University of York, UK
//
//   FSF API web pages: http://marte.unican.es/fsf/docs
//                      http://shark.sssup.it/contrib/first/docs/
//
//  This file is part of FRSH API
//
//  FRSH API is free software; you can  redistribute it and/or  modify
//  it under the terms of  the GNU General Public License as published by
//  the Free Software Foundation;  either  version 2, or (at  your option)
//  any later version.
//
//  FRSH API  is distributed  in  the hope  that  it  will  be useful,  but
//  WITHOUT  ANY  WARRANTY;     without  even the   implied   warranty  of
//  MERCHANTABILITY  or  FITNESS FOR  A  PARTICULAR PURPOSE. See  the  GNU
//  General Public License for more details.
//
//  You should have  received a  copy of  the  GNU  General Public License
//  distributed  with  FRSH API;  see file COPYING.   If not,  write to the
//  Free Software  Foundation,  59 Temple Place  -  Suite 330,  Boston, MA
//  02111-1307, USA.
//
//  As a special exception, if you include this header file into source
//  files to be compiled, this header file does not by itself cause
//  the resulting executable to be covered by the GNU General Public
//  License.  This exception does not however invalidate any other
//  reasons why the executable file might be covered by the GNU General
//  Public License.
// -----------------------------------------------------------------------
/*-----------------------------------------------------------------------
 *                                                         V1.0  Dec 2001
 *
 *                    't i m e s p e c _ o p e r a t i o n s'
 *
 *                                      H
 *
 * File 'timespec_operations.h'                                   by MAR.
 *                           (july 2002)  transformed into macros by Jul.
 * Some basic operations with the type 'timespec'.
 *
 * ----------------------------------------------------------------------
 *  Copyright (C) 2001   Universidad de Cantabria, SPAIN
 *
 *  Authors: Mario Aldea Rivas          aldeam@ctr.unican.es
 *           Michael Gonzalez Harbour      mgh@ctr.unican.es
 *
 * MaRTE OS  is free software; you can  redistribute it and/or  modify it
 * under the terms of the GNU General Public License  as published by the
 * Free Software Foundation;  either  version 2, or (at  your option) any
 * later version.
 *
 * MaRTE OS  is distributed  in the  hope  that  it will be   useful, but
 * WITHOUT  ANY  WARRANTY;     without  even the   implied   warranty  of
 * MERCHANTABILITY  or  FITNESS FOR A  PARTICULAR PURPOSE.    See the GNU
 * General Public License for more details.
 *
 * You should have received  a  copy of  the  GNU General Public  License
 * distributed with MaRTE  OS;  see file COPYING.   If not,  write to the
 * Free Software  Foundation,  59 Temple Place  -  Suite 330,  Boston, MA
 * 02111-1307, USA.
 *
 * As a  special exception, if you  link this  unit  with other  files to
 * produce an   executable,   this unit  does  not  by  itself cause  the
 * resulting executable to be covered by the  GNU General Public License.
 * This exception does  not however invalidate  any other reasons why the
 * executable file might be covered by the GNU Public License.
 *
 *---------------------------------------------------------------------------*/
// 16-Jul-2007 SANGORRIN corrections to msec_addto_timespec and msec2timespec
// 23-Jul-2007 SANGORRIN moved t2d and d2t from frsh_time_operations
// (TODO: test for __ALL__ the functions)
// -----------------------------------------------------------------------

#ifndef	_MARTE_MISC_TIMESPEC_OPERATIONS_H_
#define _MARTE_MISC_TIMESPEC_OPERATIONS_H_

/**
 * @file timespec_operations.h
 **/

#include <time.h> // for timespec
#include <string.h> // for memset

#define smaller_timespec(t1, t2) \
 ( \
  (t1)->tv_sec < (t2)->tv_sec || ((t1)->tv_sec == (t2)->tv_sec &&   \
  (t1)->tv_nsec < (t2)->tv_nsec) \
 )

#define smaller_or_equal_timespec(t1, t2) \
 ( \
  (t1)->tv_sec < (t2)->tv_sec || ((t1)->tv_sec == (t2)->tv_sec &&    \
  (t1)->tv_nsec <= (t2)->tv_nsec) \
 )

#define incr_timespec(t1, t2) \
do { \
  (t1)->tv_sec += (t2)->tv_sec; \
  (t1)->tv_nsec += (t2)->tv_nsec; \
  if ((t1)->tv_nsec >= 1000000000) { \
    (t1)->tv_sec ++; \
    (t1)->tv_nsec -= 1000000000; \
  } \
} while (0)

#define decr_timespec(t1, t2) \
do { \
  if ((t1)->tv_nsec < (t2)->tv_nsec) { \
    (t1)->tv_sec -= (t2)->tv_sec + 1; \
    (t1)->tv_nsec = (t1)->tv_nsec + 1000000000 - (t2)->tv_nsec; \
  } else { \
    (t1)->tv_sec -= (t2)->tv_sec; \
    (t1)->tv_nsec -= (t2)->tv_nsec; \
  } \
} while (0)

#define  add_timespec( sum , t1 , t2 ) \
do { \
  (sum)->tv_sec  = (t1)->tv_sec  + (t2)->tv_sec; \
  (sum)->tv_nsec = (t1)->tv_nsec + (t2)->tv_nsec; \
  if ((sum)->tv_nsec >= 1000000000) { \
    (sum)->tv_sec ++; \
    (sum)->tv_nsec -= 1000000000; \
  } \
} while (0)

#define float_to_timespec( f1 , t1 ) \
( \
  (t1)->tv_sec = (int)(f1), \
  (t1)->tv_nsec = (int)(((f1)-(float)((t1)->tv_sec))*1000000000.0), \
  (t1) \
)

#define float_to_timespec_value(f1, t1) \
( \
  (t1).tv_sec = (int)(f1), \
  (t1).tv_nsec = (int)(((f1)-(float)((t1).tv_sec))*1000000000.0), \
  (t1) \
)

//---------------//
// msec2timespec //
//---------------//

static inline struct timespec msec2timespec(long msec)
{
    struct timespec result = {-1, -1};

    if (msec >= 1000) {
        result.tv_sec = msec/1000;
        result.tv_nsec = (msec % 1000) * 1000000;
    } else {
        result.tv_sec = 0;
        result.tv_nsec = msec * 1000000;
    }

    return result;
}

static inline long timespec2msec(const struct timespec *timespec)
{
    
    return (timespec->tv_sec % 2147482) * 1000 + timespec->tv_nsec/1000000;
}


#define HOURS_IN_MSECS 3600000
#define MINUTES_IN_MSECS 60000

#define HOURS_IN_TS     1000000
#define MINUTES_IN_TS    100000

static inline long timespec2msects(const struct timespec *timespec)
{
    long msec_total = -1;
    long minutes = -1;
    long hours = -1;

    long remainder_msecs = -1;
    long timestamp = -1;

    msec_total = (timespec->tv_sec % 2147482) * 1000 + timespec->tv_nsec/1000000;

    hours = msec_total / HOURS_IN_MSECS;
    minutes = (msec_total % HOURS_IN_MSECS) / MINUTES_IN_MSECS;
    remainder_msecs = ( (msec_total % HOURS_IN_MSECS) % MINUTES_IN_MSECS);
    
    timestamp = hours*HOURS_IN_TS + minutes*MINUTES_IN_TS + remainder_msecs;

    return timestamp;
}
    

//------------------------//
// timespec_lessthan_msec //
//------------------------//

static inline int timespec_lessthan_msec(struct timespec *timespec, long msec)
{
    struct timespec msec_timespec = {0, 0};

    msec_timespec = msec2timespec(msec);
    return smaller_timespec(timespec, &msec_timespec);
}

//---------------------//
// msec_addto_timespec //
//---------------------//

static inline void msec_addto_timespec(long msec, struct timespec *timespec)
{
    struct timespec msec_timespec = {0, 0};

    msec_timespec = msec2timespec(msec);
    incr_timespec(timespec, &msec_timespec);
}

//--------------------------//
// t2d (timespec to double) //
//--------------------------//

static inline double t2d(struct timespec time)
{
    return time.tv_nsec*0.000000001 + (double)time.tv_sec;
}

//--------------------------//
// d2t (double to timespec) //
//--------------------------//

static inline struct timespec d2t(double time)
{
    struct timespec tmp;

    tmp.tv_sec = (long) time;
    tmp.tv_nsec = (long)((time - (double)tmp.tv_sec) * 1000000000);

    return tmp;
}

#endif /* _MARTE_MISC_TIMESPEC_OPERATIONS_H_ */
