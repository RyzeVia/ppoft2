#ifndef __UTILMACROS_J_H_
#define __UTILMACROS_J_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <time.h>
#include <math.h>
#include <limits.h>

// global value chain
#ifdef    GLOBAL_DEFINITION
#define   GLOBAL
#define   GLOBAL_VAL(v)  = (v)
#define   GLOBAL_STVAL(v...)  = {v}
#else
#define   GLOBAL	extern
#define   GLOBAL_VAL(v)
#define   GLOBAL_STVAL(v...)
#endif

#ifndef ERROR_EXIT
#define ERROR_EXIT() exit(EXIT_FAILURE)
#endif


// error message chain
#define LOCSTR_SIZE	32
#define PERRORF(size, msgs...)\
	do{\
		char errstr[size];\
		snprintf(errstr, size, msgs);\
		perror(errstr);\
	}while(0)
#define PERROR_LOC(msg)\
		PERRORF(strlen(msg) + LOCSTR_SIZE, "%s[%s,%d]", msg, __FILE__, __LINE__)
#define ERRCHK(exp, msg)\
	do{\
		if ((exp) > 0){\
			PERROR_LOC(msg);\
			ERROR_EXIT();\
		}\
	}while(0)
#define ERRORF(msgs...)\
	do{\
		fprintf(stderr, msgs);\
		fflush(stderr);\
	}while(0)
#define ERROR_LOC(msg)\
	do{\
		ERRORF("%s[%s,%d]%s\n", __FUNCTION__, __FILE__, __LINE__, msg);\
	}while(0)

// debug message
#ifdef DEBUG_ENABLE

#ifndef DEBUG_LENGTH
#define DEBUG_LENGTH 64
#endif

// debug level default definition
#ifndef DEBUG_LEVEL
#define DEBUG_LEVEL	100
#endif
#define DEBUG_LEVEL_MUST	0
#define DEBUG_LEVEL_FATAL	20
#define DEBUG_LEVEL_ERROR	40
#define DEBUG_LEVEL_WARNING	60
#define DEBUG_LEVEL_INFO	80
#define DEBUG_LEVEL_TRIVIAL	100

// debug topic default definition
#ifndef DEBUG_TOPIC
#define DEBUG_TOPIC	-1
#endif
#define DEBUG_TOPIC_NETWORK		60
#define DEBUG_TOPIC_PASSTHROUGH	80
#define DEBUG_TOPIC_TRIVIAL		100

#define DEBUG_EXPR_LEVEL(level)\
	(level < DEBUG_LEVEL)

#define DEBUG_EXPR_TOPIC(topic)\
	(topic == DEBUG_TOPIC)

#define STATIC_DEBUGPRINT(expr, msgs...)\
	do{\
		if (expr) fprintf(stderr, msgs);\
	}while(0)

#define DEBUGF(level, msgs...)\
	do{\
		STATIC_DEBUGPRINT(DEBUG_EXPR_LEVEL(level), msgs);\
	}while(0)

#define DEBUGTOPF(topic, msgs...)\
	do{\
		STATIC_DEBUGPRINT(DEBUG_EXPR_TOPIC(topic), msgs);\
	}while(0)

#define DEBUG_LOC(level, msgs...)\
	do{\
		time_t timet;\
		struct tm *tm;\
		char deblocfbuf[DEBUG_LENGTH];\
		snprintf(deblocfbuf, DEBUG_LENGTH, msgs);\
		time(&timet);\
		tm = localtime(&timet);\
		DEBUGF(level, "%2d/%02d %2d:%02d:%02d[%s,%d]:%s\n",\
				tm->tm_mon + 1, tm->tm_mday,\
				tm->tm_hour, tm->tm_min, tm->tm_sec, __FILE__, __LINE__, deblocfbuf);\
	}while(0)

#define DEBUGTOP_LOC(topic, msgs...)\
	do{\
		time_t timet;\
		struct tm *tm;\
		char deblocfbuf[DEBUG_LENGTH];\
		snprintf(deblocfbuf, DEBUG_LENGTH, msgs);\
		time(&timet);\
		tm = localtime(&timet);\
		DEBUGTOPF(top, "%2d/%02d %2d:%02d:%02d[%s,%d]:%s\n",\
				tm->tm_mon + 1, tm->tm_mday,\
				tm->tm_hour, tm->tm_min, tm->tm_sec, __FILE__, __LINE__, deblocfbuf);\
	}while(0)

#define DEBCHK(level, exp, msgs...)\
	do{\
		if ((exp) > 0){\
			DEBUG_LOC(level, msgs);\
		}\
	}while(0)

#define DEBTOPCHK(topic, exp, msgs...)\
	do{\
		if ((exp) > 0){\
			DEBUGTOP_LOC(topic, msgs);\
		}\
	}while(0)

#else
#define DEBUGF(level, msgs...)
#define DEBUG_LOC(level, msg)
#define DEBCHK(level, exp, msg...)
#define DEBUGTOPF(topic, msgs...)
#define DEBUGTOP_LOC(topic, msg...)
#define DEBTOPCHK(topic, exp, msg...)
#endif

// malloc chain
#define MALLOC(type, vp, size)\
	do{\
		vp = (type) malloc((size));\
		ERRCHK((vp) == NULL, "malloc");\
	}while(0)
#define CALLOC(type, vp, count, size)\
	do{\
		vp = (type) calloc((count), (size));\
		ERRCHK((vp) == NULL, "calloc");\
	}while(0)

// min-max chain

// frequent use functions
//

void misc_dbl2tv(double wt, struct timeval *tv);
double misc_tv2dbl(struct timeval *tv);
void misc_dbl2ts(double wt, struct timespec *ts);
double misc_ts2dbl(struct timespec *ts);
double misc_unit2dbl(char* unit);
#define TVPRINTF(tv_p) (int)((tv_p)->tv_sec), (long int)((tv_p)->tv_usec)
#define TVSCANF(tv_p) (int*)(&((tv_p)->tv_sec)), (long int*)(&((tv_p)->tv_usec))
#define TSPRINTF(tv_p) (int)((tv_p)->tv_sec), (long int)((tv_p)->tv_nsec)
#define TSSCANF(tv_p) (int*)(&((tv_p)->tv_sec)), (long int*)(&((tv_p)->tv_nsec))

/* Timespec macro */
#define tsclear(ts_p) ((ts_p)->tv_sec = (ts_p)->tv_nsec = 0)
#define tsisset(ts_p) ((ts_p)->tv_sec || (ts_p)->tv_nsec)
#define tscmp(a_p, b_p, CMP) \
	(((a_p)->tv_sec == (b_p)->tv_sec) ? \
     ((a_p)->tv_nsec CMP (b_p)->tv_nsec) : \
     ((a_p)->tv_sec CMP (b_p)->tv_sec))
#define tsabs(a_p, result_p) \
	do { \
		struct timespec ___tvzero; \
		tsclear(&___tvzero); \
		if (tscmp(a_p, &___tvzero, <)) \
		{ \
			tssub(&___tvzero, a_p, result_p); \
		} else { \
			tssub(a_p, &___tvzero, result_p); \
		} \
	} while(0)
#define tsadd(a_p, b_p, result_p) \
  do { \
    (result_p)->tv_sec = (a_p)->tv_sec + (b_p)->tv_sec; \
    (result_p)->tv_nsec = (a_p)->tv_nsec + (b_p)->tv_nsec; \
    if ((result_p)->tv_nsec >= 1000000000) \
      { \
        ++(result_p)->tv_sec; \
        (result_p)->tv_nsec -= 1000000000; \
      } \
  } while (0)
#define tssub(a_p, b_p, result_p) \
  do { \
    (result_p)->tv_sec = (a_p)->tv_sec - (b_p)->tv_sec; \
    (result_p)->tv_nsec = (a_p)->tv_nsec - (b_p)->tv_nsec; \
    if ((result_p)->tv_nsec < 0) { \
      --(result_p)->tv_sec; \
      (result_p)->tv_nsec += 1000000000; \
    } \
  } while (0)

#define tv2dbl(tv_p) \
		((double)(tv_p)->tv_sec + ((double)(tv_p)->tv_usec / 1000. / 1000.))

#define ts2dbl(ts_p) \
		((double)(ts_p)->tv_sec + ((double)((ts_p)->tv_nsec / 1000. / 1000. / 1000.)))

// timespec/val-double conversion

#define dbl2tv(srcdbl, dsttv_p) \
	do{ \
		(dsttv_p)->tv_usec = (susrconds_t) modf (srcdbl, &((dsttv_p)->tv_sec)) * 1000. * 1000.; \
	}while(0)

#define dbl2ts(srcdbl, dstts_p) \
	do{ \
		(dstts_p)->tv_nsec = (long int) modf (srcdbl, &((dstts_p)->tv_sec)) * 1000. * 1000. * 1000.; \
	}while(0)


#ifdef GLOBAL_DEFINITION
double misc_unit2dbl(char* unit) {
	double res = 1;
	if (strncasecmp(unit, "B", 1) == 0) {
		res = 1.;
	} else if (strncasecmp(unit, "KB", 2) == 0) {
		res = 1024.;
	} else if (strncasecmp(unit, "MB", 2) == 0) {
		res = 1024. * 1024.;
	} else if (strncasecmp(unit, "GB", 2) == 0) {
		res = 1024. * 1024. * 1024.;
	} else if (strncasecmp(unit, "TB", 2) == 0) {
		res = 1024. * 1024. * 1024. * 1024.;
	}
	return res;
}
#endif

// measurement chain
#ifdef	MEASURE_WTIME
<<<<<<< HEAD
#define	MWTIME_PRINT(fh_p)\
	do{\
		struct timeval tv;\
		gettimeofday(&tv, NULL);\
			fprintf(fh_p, "%f", tv2dbl(&tv));\
=======
#define	MWTIME_PRINT(fh_p,msgs)\
	do{\
		struct timeval tv;\
		gettimeofday(&tv, NULL);\
		if(msgs==NULL){\
			fprintf(fh_p, "%f", tv2dbl(&tv));\
		}else{\
			fprintf(fh_p, "%s: %f", msgs, tv2dbl(&tv));\
		}\
>>>>>>> 634e12bc7c23757ac1c593f1dbc04bb4399f79e3
	}
#else
#define MWTIME_PRINT(fh_p, msgs)
#endif

#endif /*MISCMACRO_H_*/
