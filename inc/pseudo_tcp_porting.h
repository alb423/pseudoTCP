/**
 * @file    pseudo_tcp_porting.h
 * @brief   pseudo_tcp include file
 *
 * @version $Revision$
 * @date    $Date$
 * @author  albert.liao
 *
 * @par Fixes:
 *    None.
 * @par To Fix:
 *    None.
 * @par Reference:
 *    None.
 *****************************************************************************
 *  <b>CONFIDENTIAL</b><br>
 *****************************************************************************/

#ifndef __PSEUDO_TCP_PORTING_H__
#define __PSEUDO_TCP_PORTING_H__

#ifdef __cplusplus
extern "C" {
#endif

/*** STANDARD INCLUDES *******************************************************/
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#if __LINUX__ || __linux || __unix || __unix
#include <error.h>
#endif
#include <errno.h>

/*** PROJECT INCLUDES ********************************************************/
#include "mi_types.h"

/*** MACROS ******************************************************************/
//#define CHECK_COPY_BUFFER  // For debug only

// LoggingSeverity
#define LS_ERROR     1
#define LS_WARNING   2
#define LS_INFO      3
#define LS_VERBOSE   4
#define LS_SENSITIVE 5
//#define LS_LEVEL LS_VERBOSE
#define LS_LEVEL LS_ERROR
    
// if(sev <= LS_ERROR) printf("%s, %s:%d, %s\n", __FILE__, __FUNCTION__, __LINE__, x);
    
#if 0
#define LOG_F(sev,x) \
{\
    U32 time = TimeSince(LogStartTime()); \
    if(sev <= LS_LEVEL) printf("[%03d:%03d]  %s:%d , %s \n", (time / 1000), (time % 1000), __FUNCTION__, __LINE__, x); \
}
    
#define LOG(sev, x)\
{\
    U32 time = TimeSince(LogStartTime()); \
    if(sev <= LS_LEVEL) printf("[%03d:%03d] %s \n", (time / 1000), (time % 1000), x); \
}
    
#define LOG_ARG(sev,x) \
    if(sev <= LS_LEVEL) printf("%s:%d, %s 0x%08x\n",__FUNCTION__, __LINE__, #x, (U32)x);

#else
    #define LOG(sev,x)
    #define LOG_F(sev,x)
    #define LOG_ARG(sev,x)
#endif
        
//#define LOG_F(sev,x)

/*
inline bool Assert(bool result, const char* function, const char* file,
                   int line, const char* expression) {
   if (!result) {
      fprintf(stderr, "%s %s:%d %s\n", function, file, line, expression);
      // On POSIX systems, SIGTRAP signals debuggers to break without killing the
      // process. If a debugger isn't attached, the uncaught SIGTRAP will crash the
      // app.
      raise(SIGTRAP);
      return false;
   }
   return true;
}
*/

#ifndef VERIFY
#define VERIFY(x) Assert((x), __FUNCTION__, __FILE__, __LINE__, #x)
#endif

#define ASSERT(x) VERIFY(x)
//#define ASSERT(x) fprintf(stderr, "__FILE__, __FUNCTION__, __LINE__, ##x");

/*** GLOBAL TYPES DEFINITIONS ************************************************/


/*** PRIVATE TYPES DEFINITIONS ***********************************************/


/*** GLOBAL VARIABLE DECLARATIONS (EXTERN) ***********************************/
extern U32 Time();
extern S32 TimeDiff(U32 later, U32 earlier);
extern U32 TimeAfter(S32 elapsed);
extern S32 TimeSince(U32 earlier);
extern S32 TimeUntil(U32 later);
extern bool TimeIsLater(U32 earlier, U32 later);
extern U32 LogStartTime();
extern U32 LogStartTime_Init();
extern bool Assert(bool result, const char* function, const char* file,
                   int line, const char* expression);
/*** PUBLIC FUNCTION PROTOTYPES **********************************************/



/** Doubly Linked List ******************************************************/


/*****************************************************************************/

#ifdef __cplusplus
}
#endif

#endif /* End of #ifndef __PSEUDO_TCP_PORTING_H__ */

/******************************************************************************
 *  CONFIDENTIAL
 *****************************************************************************/
