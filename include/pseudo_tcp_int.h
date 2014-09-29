/**
 * @file    pseudo_tcp.h
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

#ifndef __PSEUDO_TCP_INT_H__
#define __PSEUDO_TCP_INT_H__

#ifdef __cplusplus
extern "C" {
#endif

/*** STANDARD INCLUDES *******************************************************/
#ifdef __LINUX__
#include <error.h>
#endif

/*** PROJECT INCLUDES ********************************************************/


/*** MACROS ******************************************************************/


/*** GLOBAL TYPES DEFINITIONS ************************************************/
typedef enum tStreamState { SS_CLOSED, SS_OPENING, SS_OPEN } tStreamState;
typedef enum tStreamResult { SR_ERROR, SR_SUCCESS, SR_BLOCK, SR_EOS } tStreamResult;

/** Doubly Linked List ******************************************************/


/*****************************************************************************/

#ifdef __cplusplus
}
#endif

#endif /* End of #ifndef __PSEUDO_TCP_H__ */

/******************************************************************************
 *  CONFIDENTIAL
 *****************************************************************************/
