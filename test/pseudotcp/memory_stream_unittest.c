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
#include "fifo_buffer.h"
#include "memory_stream.h"
/*** MACROS ******************************************************************/


/*** GLOBAL VARIABLE DECLARATIONS (EXTERN) ***********************************/


/*** PRIVATE TYPES DEFINITIONS ***********************************************/


/*** PRIVATE VARIABLE DECLARATIONS (STATIC) **********************************/


/*** PRIVATE FUNCTION PROTOTYPES *********************************************/


/*** PUBLIC FUNCTION DEFINITIONS *********************************************/

// Test Memory Stream (mimic pseudotcp_unittest.cc::TestTransfer())
void MemoryStreamTest_TestTransfer(void)
{
    int size = 10000;
    int i = 0;
    char ch;
    size_t vReceived = 0;
    size_t bytes_written = 0, bytes_read = 0;
    bool bFlag = false;

    tMemoryStream *pSendStream = MS_Init(NULL, 10000);
    tMemoryStream *pRecvStream = MS_Init(NULL, 10000);
    CU_ASSERT_PTR_NOT_NULL(pSendStream);
    CU_ASSERT_PTR_NOT_NULL(pRecvStream);

    bFlag = MS_ReserveSize(pSendStream, size);
    CU_ASSERT_EQUAL(true, bFlag);
    bFlag = MS_ReserveSize(pRecvStream, size);
    CU_ASSERT_EQUAL(true, bFlag);

    // mimic the usage in psuedotcp_unittest.c

    MS_Rewind(pSendStream);
    for (i = 0; i < size; ++i) {
        ch = (char)i;
        MS_Write(pSendStream, &ch, 1, &bytes_written, NULL);
        //CU_ASSERT_EQUAL(1, bytes_written);
    }

    MS_Rewind(pSendStream);
    for (i = 0; i < size; ++i) {
        MS_Read(pSendStream, &ch, 1, &bytes_read, NULL);
        //CU_ASSERT_EQUAL(1, bytes_read);
        MS_Write(pRecvStream, &ch, 1, NULL, NULL);
    }

    MS_GetSize(pRecvStream, &vReceived);
    CU_ASSERT_EQUAL(0, memcmp(MS_GetBuffer(pSendStream), MS_GetBuffer(pRecvStream), size));

    MS_Destroy(pRecvStream);
    MS_Destroy(pSendStream);
}

/*** Test for List *********************************************/



/*** Test for PseudoTCP *********************************************/

/*****************************************************************************/

#ifdef __cplusplus
}
#endif

/**  @} */
/******************************************************************************
 *  CONFIDENTIAL
 *****************************************************************************/
