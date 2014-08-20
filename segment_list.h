/**
 * @file    SEGMENT_LIST_h
 * @brief   segment_list include file
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

#ifndef __SEGMENT_LIST_H__
#define __SEGMENT_LIST_H__

#ifdef __cplusplus
extern "C" {
#endif

/*** STANDARD INCLUDES *******************************************************/


/*** PROJECT INCLUDES ********************************************************/
#include "mi_types.h"
#include "mi_util.h"
/*** MACROS ******************************************************************/


/*** GLOBAL TYPES DEFINITIONS ************************************************/

/*
typedef enum { eSSegment, eRSegment } tSegmentType;

typedef struct RSegment {
   U32 seq;
   U32 len;
} tRSegment;

typedef struct SSegment {
   U32 seq;
   U32 len;
   //U32 tstamp;
   U8 xmit;
   bool bCtrl;
} tSSegment;
*/

typedef struct tRSSegment
{
   tMI_DLNODE DLNode;
   U32 seq;
   U32 len;
   U8 xmit;
   bool bCtrl;
} tRSSegment;

/*** PRIVATE TYPES DEFINITIONS ***********************************************/

/*** GLOBAL VARIABLE DECLARATIONS (EXTERN) ***********************************/
// Reference http://www.cplusplus.com/reference/list/list/

extern void SEGMENT_LIST_Init(tMI_DLIST *pDList); 

// Returns an iterator pointing to the first element in the list container.
extern tRSSegment *SEGMENT_LIST_begin(tMI_DLIST *pDList);

// Returns an iterator referring to the past-the-end element in the list container.
extern tRSSegment *SEGMENT_LIST_end(tMI_DLIST *pDList);

// Returns a reference to the first element in the list container.
extern tRSSegment *SEGMENT_LIST_front(tMI_DLIST *pDList);

// Returns a reference to the last element in the list container.
extern tRSSegment *SEGMENT_LIST_back(tMI_DLIST *pDList);

// Inserts a new element at the beginning of the list, right before its current first element. 
extern void SEGMENT_LIST_push_front(tMI_DLIST *pDList, tMI_DLNODE *pNode) ;

// Removes the first element in the list container.  
extern void SEGMENT_LIST_pop_front(tMI_DLIST *pList);

// Adds a new element at the end of the list container, after its current last element.
extern void SEGMENT_LIST_push_back(tMI_DLIST *pList, tMI_DLNODE *pNode);

// Removes the last element in the list container
extern void SEGMENT_LIST_pop_back(tMI_DLIST *pList);

// Test whether container is empty 
extern bool SEGMENT_LIST_empty(tMI_DLIST *pDList); 

// Return size
extern U32 SEGMENT_LIST_size(tMI_DLIST *pDList);

// Delete current element
extern tMI_DLNODE *SEGMENT_LIST_erase(tMI_DLIST *pDList, tMI_DLNODE *pNode);

// inserting new elements before the element at the specified position
extern void SEGMENT_LIST_insert(tMI_DLIST *pDList, tMI_DLNODE *pNode, tMI_DLNODE *pNode2);

/*** PUBLIC FUNCTION PROTOTYPES **********************************************/


/** Doubly Linked List ******************************************************/


/*****************************************************************************/

#ifdef __cplusplus
}
#endif

#endif /* End of #ifndef __SEGMENT_LIST_H__ */

/******************************************************************************
 *  CONFIDENTIAL
 *****************************************************************************/
