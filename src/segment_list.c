/**
 * @file    segment_list.c
 * @brief   pseudoTCP
 *
 *
 * @version $Revision$
 * @date    $Date$
 * @author  albert.liao
 *
 * @par Fixes:
 *   None.
 * @par To Fix:
 * @par Reference:
 *****************************************************************************
 *  <b>CONFIDENTIAL</b><br>
 *****************************************************************************/

/** @addtogroup pseudoTCP
 *  @addtogroup pseudoTCP Utilities
 *  @ingroup pseudoTCP
 *  @{
 */

#ifdef __cplusplus
extern "C" {
#endif

/*** STANDARD INCLUDES *******************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/*** PROJECT INCLUDES ********************************************************/
#include "segment_list.h"
#include "pseudo_tcp_porting.h"
/*** MACROS ******************************************************************/


/*** GLOBAL VARIABLE DECLARATIONS (EXTERN) ***********************************/


/*** PRIVATE TYPES DEFINITIONS ***********************************************/


/*** PRIVATE VARIABLE DECLARATIONS (STATIC) **********************************/


/*** PRIVATE FUNCTION PROTOTYPES *********************************************/


/*** PUBLIC FUNCTION DEFINITIONS *********************************************/
void SEGMENT_LIST_Init(tMI_DLIST *pDList)
{
    if(pDList!=NULL)
        MI_DlInit(pDList);
}

// Returns an iterator pointing to the first element in the list container.
tRSSegment *SEGMENT_LIST_begin(tMI_DLIST *pDList)
{
    if(pDList) {
        tMI_DLNODE *vpNode = MI_DlFirst(pDList);
        if(vpNode) {
            tRSSegment *vpSegment;
            vpSegment = MI_NODEENTRY(vpNode, tRSSegment, DLNode);
            return vpSegment;
        }
    }
    return NULL;
}

// Returns an iterator referring to the past-the-end element in the list container.
tRSSegment *SEGMENT_LIST_end(tMI_DLIST *pDList)
{
    if(pDList) {
        tMI_DLNODE *vpNode = MI_DlLast(pDList);
        if(vpNode) {
            tRSSegment *vpSegment;
            vpSegment = MI_NODEENTRY(vpNode, tRSSegment, DLNode);
            return vpSegment;
        }
    }
    return NULL;
}

// Test whether container is empty
bool SEGMENT_LIST_empty(tMI_DLIST *pDList)
{
    if(pDList) {
        if(MI_DlCount(pDList)==0) {
            return true;
        }
        return false;
    }
    return true;
}

// Return size
U32 SEGMENT_LIST_size(tMI_DLIST *pDList)
{
    return MI_DlCount(pDList);
}

// ==== Element Access ====
// Returns a reference to the first element in the list container.
tRSSegment *SEGMENT_LIST_front(tMI_DLIST *pDList)
{
    if(pDList) {
        tMI_DLNODE *vpNode = MI_DlFirst(pDList);
        if(vpNode) {
            tRSSegment *vpSegment;
            vpSegment = MI_NODEENTRY(vpNode, tRSSegment, DLNode);
            return vpSegment;
        }
    }
    return NULL;
}

// Returns a reference to the last element in the list container.
tRSSegment *SEGMENT_LIST_back(tMI_DLIST *pDList)
{
    if(pDList) {
        tMI_DLNODE *vpNode = MI_DlLast(pDList);
        if(vpNode) {
            tRSSegment *vpSegment;
            vpSegment = MI_NODEENTRY(vpNode, tRSSegment, DLNode);
            return vpSegment;
        }
    }
    return NULL;
}

// Inserts a new element at the beginning of the list, right before its current first element.
void SEGMENT_LIST_push_front(tMI_DLIST *pDList, tMI_DLNODE *pNode)
{
    MI_DlPushHead(pDList, pNode);
}

// Removes the first element in the list container.
void SEGMENT_LIST_pop_front(tMI_DLIST *pDList)
{
    MI_DlPopHead(pDList);
//    tMI_DLNODE *pNode = MI_DlPopHead(pDList);
//    LOG_ARG(LS_SENSITIVE, pNode);
}

// Adds a new element at the end of the list container, after its current last element.
void SEGMENT_LIST_push_back(tMI_DLIST *pDList, tMI_DLNODE *pNode)
{
    LOG_ARG(LS_SENSITIVE, pNode);
    MI_DlPushTail(pDList, pNode);
}

// Removes the last element in the list container
void SEGMENT_LIST_pop_back(tMI_DLIST *pDList)
{
    MI_DlPopTail(pDList);
}

// Delete current element
tMI_DLNODE *SEGMENT_LIST_erase(tMI_DLIST *pDList, tMI_DLNODE *pNode)
{
    LOG_ARG(LS_SENSITIVE, pNode);
    if(pDList) {
        U32 vIndex;
        tMI_DLNODE *vpNode = NULL;

        vIndex = MI_DlIndex(pDList, pNode);
        if(vIndex == MI_UTIL_ERROR)
            return NULL;

        vpNode = MI_DlNth(pDList, vIndex+1);
        if(vpNode == NULL) {
            // This is the last node in list
            // return NULL;
        }

        MI_DlDelete(pDList, pNode);

        return vpNode;
    }

    return NULL;
}


// inserting new elements after the element at the specified position
void SEGMENT_LIST_insertBefore(tMI_DLIST *pDList, tMI_DLNODE *pNode, tMI_DLNODE *pNode2)
{
    LOG_ARG(LS_SENSITIVE, pNode2);
    MI_DlInsertBefore(pDList, pNode, pNode2);
}

// inserting new elements before the element at the specified position
void SEGMENT_LIST_insertAfter(tMI_DLIST *pDList, tMI_DLNODE *pNode, tMI_DLNODE *pNode2)
{
    LOG_ARG(LS_SENSITIVE, pNode);    
    LOG_ARG(LS_SENSITIVE, pNode2);
    MI_DlInsertAfter(pDList, pNode, pNode2);
}
    
void SEGMENT_LIST_Dump(tMI_DLIST *pDList)
{
#if LS_LEVEL == LS_SENSITIVE
    if(pDList) {
        printf("Dump list count=%d (node=0x%08x) (next=0x%08x) (prev=0x%08x)\n", pDList->count, (U32)&pDList->node,
               (U32)pDList->node.next, (U32)pDList->node.prev);
        
        tMI_DLNODE *vpNode = MI_DlFirst(pDList);
        tRSSegment *it;
        
        while(vpNode) {

            it = MI_NODEENTRY(vpNode, tRSSegment, DLNode);
            
            //printf("xmit=%d node=0x%08x (next=0x%08x) (prev=0x%08x)\n", it->xmit, (U32)vpNode, (U32)vpNode->next, (U32)vpNode->prev);
            
            if(vpNode->next!=0) {
                vpNode = vpNode->next;
            }
            else
                break;
        }
    }
#endif
}
/*****************************************************************************/

#ifdef __cplusplus
}
#endif

/**  @} */
/******************************************************************************
 *  CONFIDENTIAL
 *****************************************************************************/
