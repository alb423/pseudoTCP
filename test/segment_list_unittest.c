/**
 * @file    pseudo_tcp_unittest.c
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
#include <CUnit/CUnit.h>
#include <CUnit/Console.h>
#include "mi_types.h"
#include "mi_util.h"
#include "segment_list.h"
/*** MACROS ******************************************************************/


/*** GLOBAL VARIABLE DECLARATIONS (EXTERN) ***********************************/


/*** PRIVATE TYPES DEFINITIONS ***********************************************/


/*** PRIVATE VARIABLE DECLARATIONS (STATIC) **********************************/


/*** PRIVATE FUNCTION PROTOTYPES *********************************************/


/*** PUBLIC FUNCTION DEFINITIONS *********************************************/

typedef struct tTestCtxt {
    tMI_DLNODE DLNode;
    U32 value;
} tTestCtxt;


/*** Test for List *********************************************/
/*
   Push 3 Ctxt at tail
   Pop 3 Ctxt from head
   The order should be the same
*/
void Test_DoubleLinkList_s01(void)
{
    tMI_DLIST vDList;
    tTestCtxt vTCtxt[3];
    U32 vI = 0;
    tMI_DLNODE * vpNode;
    tTestCtxt * vpTCtxt;

    MI_DlInit(&vDList);

    for (vI = 0; vI < 3; vI++) {
        vTCtxt[vI].value = vI;
    }

    MI_DlPushTail(&vDList, &vTCtxt[0].DLNode);
    MI_DlPushTail(&vDList, &vTCtxt[2].DLNode);
    MI_DlPushTail(&vDList, &vTCtxt[1].DLNode);

    vpNode = MI_DlPopHead(&vDList);
    CU_ASSERT_NOT_EQUAL(vpNode, NULL);

    if (vpNode != NULL) {
        vpTCtxt = MI_NODEENTRY(vpNode, tTestCtxt, DLNode);
        CU_ASSERT_EQUAL(vpTCtxt->value, 0);
    }

    vpNode = MI_DlPopHead(&vDList);
    CU_ASSERT_NOT_EQUAL(vpNode, NULL);

    if (vpNode != NULL) {
        vpTCtxt = MI_NODEENTRY(vpNode, tTestCtxt, DLNode);
        CU_ASSERT_EQUAL(vpTCtxt->value, 2);
    }

    vpNode = MI_DlPopHead(&vDList);
    CU_ASSERT_NOT_EQUAL(vpNode, NULL);

    if (vpNode != NULL) {
        vpTCtxt = MI_NODEENTRY(vpNode, tTestCtxt, DLNode);
        CU_ASSERT_EQUAL(vpTCtxt->value, 1);
    }

    vpNode = MI_DlPopHead(&vDList);
    CU_ASSERT_EQUAL(vpNode, NULL);
}

/*
   Push 3 (0, 2, 4)Ctxt at tail
   insert 1 after 0
   insert 3 after 2
   delete 0
   Pop all from head and the order should be 1, 2, 3, 4
*/
void Test_DoubleLinkList_s02(void)
{
    tMI_DLIST vDList;
    tTestCtxt vTCtxt[5];
    U32 vI = 0;
    tMI_DLNODE * vpNode;
    tTestCtxt * vpTCtxt;

    MI_DlInit(&vDList);

    for (vI = 0; vI < 5; vI++) {
        vTCtxt[vI].value = vI;
    }

    MI_DlPushTail(&vDList, &vTCtxt[0].DLNode);
    MI_DlPushTail(&vDList, &vTCtxt[2].DLNode);
    MI_DlPushTail(&vDList, &vTCtxt[4].DLNode);

    MI_DlInsertAfter(&vDList, &vTCtxt[0].DLNode, &vTCtxt[1].DLNode);
    MI_DlInsertAfter(&vDList, &vTCtxt[2].DLNode, &vTCtxt[3].DLNode);

    MI_DlDelete(&vDList, &vTCtxt[0].DLNode);

    for (vI = 1; vI < 5; vI++) {
        vpNode = MI_DlPopHead(&vDList);
        CU_ASSERT_NOT_EQUAL(vpNode, NULL);

        if (vpNode != NULL) {
            vpTCtxt = MI_NODEENTRY(vpNode, tTestCtxt, DLNode);
            CU_ASSERT_EQUAL(vpTCtxt->value, vI);
        }
    }

    vpNode = MI_DlPopHead(&vDList);
    CU_ASSERT_EQUAL(vpNode, NULL);
}

const void *Test_DoubleLinkList_s03_keyof(const tMI_DLNODE *pNode)
{
    tTestCtxt * vpTCtxt;

    vpTCtxt = MI_NODEENTRY(pNode, tTestCtxt, DLNode);

    return &vpTCtxt->value;
}

S32 Test_DoubleLinkList_s03_compare(const void *pAValue, const void *pBValue)
{
    U32 *vpAValue, *vpBValue;

    vpAValue = (U32 *) pAValue;
    vpBValue = (U32 *) pBValue;

    if (*vpAValue < *vpBValue) {
        return 1;
    } else if (*vpAValue == *vpBValue) {
        return 0;
    } else {
        return -1;
    }
}

/*
   use MI_DlInsert with 2, 3, 0, 4, 1
   Pop all from head and the order should be 0, 1, 2, 3, 4
*/
void Test_DoubleLinkList_s03(void)
{
    tMI_DLIST vDList;
    tTestCtxt vTCtxt[5];
    U32 vI = 0;
    tMI_DLNODE * vpNode;
    tTestCtxt * vpTCtxt;

    MI_DlInit(&vDList);

    for (vI = 0; vI < 5; vI++) {
        vTCtxt[vI].value = vI;
    }

    MI_DlInsert(&vDList, &vTCtxt[2].DLNode, Test_DoubleLinkList_s03_keyof, Test_DoubleLinkList_s03_compare);
    MI_DlInsert(&vDList, &vTCtxt[3].DLNode, Test_DoubleLinkList_s03_keyof, Test_DoubleLinkList_s03_compare);
    MI_DlInsert(&vDList, &vTCtxt[0].DLNode, Test_DoubleLinkList_s03_keyof, Test_DoubleLinkList_s03_compare);
    MI_DlInsert(&vDList, &vTCtxt[4].DLNode, Test_DoubleLinkList_s03_keyof, Test_DoubleLinkList_s03_compare);
    MI_DlInsert(&vDList, &vTCtxt[1].DLNode, Test_DoubleLinkList_s03_keyof, Test_DoubleLinkList_s03_compare);

    for (vI = 0; vI < 5; vI++) {
        vpNode = MI_DlPopHead(&vDList);
        CU_ASSERT_NOT_EQUAL(vpNode, NULL);

        if (vpNode != NULL) {
            vpTCtxt = MI_NODEENTRY(vpNode, tTestCtxt, DLNode);
            CU_ASSERT_EQUAL(vpTCtxt->value, vI);
        }
    }

    vpNode = MI_DlPopHead(&vDList);
    CU_ASSERT_EQUAL(vpNode, NULL);
}

S32 Test_DoubleLinkList_s04_compare(const void *pAValue, const void *pBValue)
{
    U32 *vpAValue, *vpBValue;

    vpAValue = (U32 *) pAValue;
    vpBValue = (U32 *) pBValue;

    if (*vpAValue > *vpBValue) {
        return 1;
    } else if (*vpAValue == *vpBValue) {
        return 0;
    } else {
        return -1;
    }
}

/*
   use MI_DlInsert with 2, 3, 0, 4, 1
   Pop all from head and the order should be 0, 1, 2, 3, 4
*/
void Test_DoubleLinkList_s04(void)
{
    tMI_DLIST vDList;
    tTestCtxt vTCtxt[5];
    U32 vI = 0;
    tMI_DLNODE * vpNode;
    tTestCtxt * vpTCtxt;

    MI_DlInit(&vDList);

    for (vI = 0; vI < 5; vI++) {
        vTCtxt[vI].value = vI;
    }

    MI_DlPushTail(&vDList, &vTCtxt[2].DLNode);
    MI_DlPushTail(&vDList, &vTCtxt[3].DLNode);
    MI_DlPushTail(&vDList, &vTCtxt[0].DLNode);
    MI_DlPushTail(&vDList, &vTCtxt[4].DLNode);
    MI_DlPushTail(&vDList, &vTCtxt[1].DLNode);

    MI_DlSort(&vDList, Test_DoubleLinkList_s03_keyof, Test_DoubleLinkList_s04_compare);

    for (vI = 0; vI < 5; vI++) {
        vpNode = MI_DlPopHead(&vDList);
        CU_ASSERT_NOT_EQUAL(vpNode, NULL);

        if (vpNode != NULL) {
            vpTCtxt = MI_NODEENTRY(vpNode, tTestCtxt, DLNode);
            CU_ASSERT_EQUAL(vpTCtxt->value, vI);
        }
    }

    vpNode = MI_DlPopHead(&vDList);
    CU_ASSERT_EQUAL(vpNode, NULL);
}


/*** Test for Segment List *********************************************/
/*
   Push 3 Ctxt at tail
   Pop 3 Ctxt from head
   The order should be the same
*/
void Test_SegmentList_s01(void)
{
    bool bFlag;
    tMI_DLIST vDList;
    tRSSegment vTCtxt[3];
    U32 vI = 0;
    tRSSegment * vpTCtxt;

    SEGMENT_LIST_Init(&vDList);

    for (vI = 0; vI < 3; vI++) {
        vTCtxt[vI].seq = vI;
    }

    SEGMENT_LIST_push_back(&vDList, &vTCtxt[0].DLNode);
    SEGMENT_LIST_push_back(&vDList, &vTCtxt[2].DLNode);
    SEGMENT_LIST_push_back(&vDList, &vTCtxt[1].DLNode);

    vpTCtxt = SEGMENT_LIST_back(&vDList);
    CU_ASSERT_EQUAL(vpTCtxt->seq, 1);

    vpTCtxt = SEGMENT_LIST_end(&vDList);
    CU_ASSERT_EQUAL(vpTCtxt->seq, 1);

    vpTCtxt = SEGMENT_LIST_begin(&vDList);
    CU_ASSERT_EQUAL(vpTCtxt->seq, 0);

    vpTCtxt = SEGMENT_LIST_front(&vDList);
    CU_ASSERT_EQUAL(vpTCtxt->seq, 0);

    SEGMENT_LIST_pop_front(&vDList);


    vpTCtxt = SEGMENT_LIST_begin(&vDList);
    CU_ASSERT_EQUAL(vpTCtxt->seq, 2);

    vpTCtxt = SEGMENT_LIST_front(&vDList);
    CU_ASSERT_EQUAL(vpTCtxt->seq, 2);

    SEGMENT_LIST_pop_front(&vDList);

    vpTCtxt = SEGMENT_LIST_begin(&vDList);
    CU_ASSERT_EQUAL(vpTCtxt->seq, 1);

    vpTCtxt = SEGMENT_LIST_front(&vDList);
    CU_ASSERT_EQUAL(vpTCtxt->seq, 1);

    SEGMENT_LIST_pop_front(&vDList);

    vpTCtxt = SEGMENT_LIST_begin(&vDList);
    CU_ASSERT_EQUAL(vpTCtxt, NULL);

    bFlag = SEGMENT_LIST_empty(&vDList);
    CU_ASSERT_EQUAL(bFlag, true);
}

/*
   Push 3 (0, 2, 4)Ctxt at tail
   insert 1 after 0
   insert 3 after 2
   delete 0
   Pop all from head and the order should be 1, 2, 3, 4
*/
void Test_SegmentList_s02(void)
{
    bool bFlag;
    tMI_DLIST vDList;
    tRSSegment vTCtxt[5];
    U32 vI = 0;
    tRSSegment * vpTCtxt;

    SEGMENT_LIST_Init(&vDList);

    for (vI = 0; vI < 5; vI++) {
        vTCtxt[vI].seq = vI;
    }

    SEGMENT_LIST_push_back(&vDList, &vTCtxt[0].DLNode);
    SEGMENT_LIST_push_back(&vDList, &vTCtxt[2].DLNode);
    SEGMENT_LIST_push_back(&vDList, &vTCtxt[4].DLNode);

    SEGMENT_LIST_insert(&vDList, &vTCtxt[0].DLNode, &vTCtxt[1].DLNode);
    SEGMENT_LIST_insert(&vDList, &vTCtxt[2].DLNode, &vTCtxt[3].DLNode);

    vpTCtxt = SEGMENT_LIST_back(&vDList);
    CU_ASSERT_EQUAL(vpTCtxt->seq, 4);

    vpTCtxt = SEGMENT_LIST_end(&vDList);
    CU_ASSERT_EQUAL(vpTCtxt->seq, 4);

    SEGMENT_LIST_erase(&vDList, &vTCtxt[0].DLNode);

    for (vI = 1; vI < 5; vI++) {
        vpTCtxt = SEGMENT_LIST_begin(&vDList);
        CU_ASSERT_NOT_EQUAL(vpTCtxt, NULL);
        CU_ASSERT_EQUAL(vpTCtxt->seq, vI);

        vpTCtxt = SEGMENT_LIST_front(&vDList);
        CU_ASSERT_EQUAL(vpTCtxt->seq, vI);

        SEGMENT_LIST_pop_front(&vDList);
    }

    vpTCtxt = SEGMENT_LIST_begin(&vDList);
    CU_ASSERT_EQUAL(vpTCtxt, NULL);

    bFlag = SEGMENT_LIST_empty(&vDList);
    CU_ASSERT_EQUAL(bFlag, true);
}


void Test_SegmentList_s03(void)
{
    bool bFlag;
    tMI_DLIST vDList;
    tRSSegment *pTCtxt[5];
    U32 vI = 0;
    tRSSegment * vpTCtxt;

    SEGMENT_LIST_Init(&vDList);

    for (vI = 0; vI < 5; vI++) {
        pTCtxt[vI] = malloc(sizeof(tRSSegment));
        pTCtxt[vI]->seq = vI;
    }

    SEGMENT_LIST_push_back(&vDList, &pTCtxt[0]->DLNode);
    SEGMENT_LIST_push_back(&vDList, &pTCtxt[2]->DLNode);
    SEGMENT_LIST_push_back(&vDList, &pTCtxt[4]->DLNode);

    SEGMENT_LIST_insert(&vDList, &pTCtxt[0]->DLNode, &pTCtxt[1]->DLNode);
    SEGMENT_LIST_insert(&vDList, &pTCtxt[2]->DLNode, &pTCtxt[3]->DLNode);

    vpTCtxt = SEGMENT_LIST_back(&vDList);
    CU_ASSERT_EQUAL(vpTCtxt->seq, 4);

    vpTCtxt = SEGMENT_LIST_end(&vDList);
    CU_ASSERT_EQUAL(vpTCtxt->seq, 4);

    SEGMENT_LIST_erase(&vDList, &pTCtxt[0]->DLNode);

    for (vI = 1; vI < 5; vI++) {
        vpTCtxt = SEGMENT_LIST_begin(&vDList);
        CU_ASSERT_NOT_EQUAL(vpTCtxt, NULL);
        CU_ASSERT_EQUAL(vpTCtxt->seq, vI);

        vpTCtxt = SEGMENT_LIST_front(&vDList);
        CU_ASSERT_EQUAL(vpTCtxt->seq, vI);

        SEGMENT_LIST_pop_front(&vDList);
        free(vpTCtxt);
    }

    vpTCtxt = SEGMENT_LIST_begin(&vDList);
    CU_ASSERT_EQUAL(vpTCtxt, NULL);

    bFlag = SEGMENT_LIST_empty(&vDList);
    CU_ASSERT_EQUAL(bFlag, true);

}


/*****************************************************************************/

#ifdef __cplusplus
}
#endif

/**  @} */
/******************************************************************************
 *  CONFIDENTIAL
 *****************************************************************************/
