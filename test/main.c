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


#include "fifo_buffer_unittest.h"
#include "segment_list_unittest.h"
#include "memory_stream_unittest.h"
#include "pseudo_tcp_unittest.h"

/*** MACROS ******************************************************************/


/*** GLOBAL VARIABLE DECLARATIONS (EXTERN) ***********************************/


/*** PRIVATE TYPES DEFINITIONS ***********************************************/


/*** PRIVATE VARIABLE DECLARATIONS (STATIC) **********************************/


/*** PRIVATE FUNCTION PROTOTYPES *********************************************/


/*** PUBLIC FUNCTION DEFINITIONS *********************************************/
static int testInit(void) {return 0;}
static int testClean(void) {return 0;}
void testAssertTrue(void);

int main(int argc, char **argv)
{
   CU_pSuite pSuite;
   printf("test\n");
   CU_initialize_registry();
    
#if 0
   // Test Successful
   pSuite = CU_add_suite("Test FIFO", testInit, testClean);
   CU_add_test(pSuite, "FifoBufferTest_FullBufferCheck", FifoBufferTest_FullBufferCheck);
   CU_add_test(pSuite, "FifoBufferTest_WriteOffsetAndReadOffset", FifoBufferTest_WriteOffsetAndReadOffset);   
   CU_add_test(pSuite, "FifoBufferTest_All", FifoBufferTest_All);

   pSuite = CU_add_suite("Test Segment List", testInit, testClean);
   CU_add_test(pSuite, "Test_DoubleLinkList_s01", Test_DoubleLinkList_s01);
   CU_add_test(pSuite, "Test_DoubleLinkList_s02", Test_DoubleLinkList_s02);
   CU_add_test(pSuite, "Test_DoubleLinkList_s03", Test_DoubleLinkList_s03);
   CU_add_test(pSuite, "Test_DoubleLinkList_s04", Test_DoubleLinkList_s04);
   CU_add_test(pSuite, "Test_SegmentList_s01", Test_SegmentList_s01);
   CU_add_test(pSuite, "Test_SegmentList_s02", Test_SegmentList_s02);
      
   pSuite = CU_add_suite("Test Memory Stream", testInit, testClean);
   CU_add_test(pSuite, "MemoryStreamTest_TestTransfer", MemoryStreamTest_TestTransfer);

#else
   // TODO     
   pSuite = CU_add_suite("Test PseudoTCP", testInit, testClean);
   CU_add_test(pSuite, "PseudoTcpTest", PseudoTcpTest_01_01);
   CU_add_test(pSuite, "PseudoTcpTest", PseudoTcpTest_01_02);
   CU_add_test(pSuite, "PseudoTcpTestPingPong", PseudoTcpTest_02);
   CU_add_test(pSuite, "PseudoTcpTestReceiveWindow", PseudoTcpTest_03);

#endif
   CU_console_run_tests();
   CU_cleanup_registry();
   return 0;    
}


void testAssertTrue(void)
{
   CU_ASSERT_TRUE(CU_TRUE);
   CU_ASSERT_TRUE(!CU_FALSE);

   //CU_ASSERT_TRUE(!CU_TRUE);
   //CU_ASSERT_TRUE(CU_FALSE);
}

/*****************************************************************************/

#ifdef __cplusplus
}
#endif

/**  @} */
/******************************************************************************
 *  CONFIDENTIAL
 *****************************************************************************/
