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
#include <unistd.h>
    
/*** PROJECT INCLUDES ********************************************************/
#include <CUnit/CUnit.h>
#include <CUnit/Console.h>
#include "mi_types.h"
#include "fifo_buffer.h"


#include "test/fifo_buffer_unittest.h"
#include "test/segment_list_unittest.h"
#include "test/memory_stream_unittest.h"
#include "test/pseudo_tcp_unittest.h"

/*** MACROS ******************************************************************/


/*** GLOBAL VARIABLE DECLARATIONS (EXTERN) ***********************************/
extern CU_ErrorCode CU_basic_run_tests(void);

/*** PRIVATE TYPES DEFINITIONS ***********************************************/


/*** PRIVATE VARIABLE DECLARATIONS (STATIC) **********************************/


/*** PRIVATE FUNCTION PROTOTYPES *********************************************/


/*** PUBLIC FUNCTION DEFINITIONS *********************************************/
static int testInit(void)
{
    return 0;
}
static int testClean(void)
{
    //sleep(1);
    return 0;
}
void testAssertTrue(void);

void unittest_main();

int main(int argc, char **argv)
{
    unittest_main();
    return 0;
}

void unittest_main()
{
    CU_pSuite pSuite;
    CU_initialize_registry();

#if 1
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

    // Basic end-to-end data transfer tests
    CU_add_test(pSuite, "PseudoTcpTest", PseudoTcpTest_TestSend);
    CU_add_test(pSuite, "PseudoTcpTest", PseudoTcpTest_TestSendWithDelay);
    CU_add_test(pSuite, "PseudoTcpTest", PseudoTcpTest_TestSendWithLoss);
    CU_add_test(pSuite, "PseudoTcpTest", PseudoTcpTest_TestSendWithDelayAndLoss);
    CU_add_test(pSuite, "PseudoTcpTest", PseudoTcpTest_TestSendWithLossAndOptNaglingOff);
    CU_add_test(pSuite, "PseudoTcpTest", PseudoTcpTest_TestSendWithLossAndOptAckDelayOff);
    CU_add_test(pSuite, "PseudoTcpTest", PseudoTcpTest_TestSendWithDelayAndOptNaglingOff);
    CU_add_test(pSuite, "PseudoTcpTest", PseudoTcpTest_TestSendWithDelayAndOptAckDelayOff);
    CU_add_test(pSuite, "PseudoTcpTest", PseudoTcpTest_TestSendRemoteNoWindowScale);
    CU_add_test(pSuite, "PseudoTcpTest", PseudoTcpTest_TestSendLocalNoWindowScale);
    CU_add_test(pSuite, "PseudoTcpTest", PseudoTcpTest_TestSendBothUseWindowScale);
    CU_add_test(pSuite, "PseudoTcpTest", PseudoTcpTest_TestSendLargeInFlight);
    CU_add_test(pSuite, "PseudoTcpTest", PseudoTcpTest_TestSendBothUseLargeWindowScale);
    CU_add_test(pSuite, "PseudoTcpTest", PseudoTcpTest_TestSendSmallReceiveBuffer);
    CU_add_test(pSuite, "PseudoTcpTest", PseudoTcpTest_TestSendVerySmallReceiveBuffer);
    
    // Ping-pong (request/response) tests
    CU_add_test(pSuite, "PseudoTcpTest", PseudoTcpTestPingPong_TestPingPong1xMtu);
    CU_add_test(pSuite, "PseudoTcpTest", PseudoTcpTestPingPong_TestPingPong3xMtu);
    CU_add_test(pSuite, "PseudoTcpTest", PseudoTcpTestPingPong_TestPingPong2xMtu);
    CU_add_test(pSuite, "PseudoTcpTest", PseudoTcpTestPingPong_TestPingPong2xMtuWithAckDelayOff);
    CU_add_test(pSuite, "PseudoTcpTest", PseudoTcpTestPingPong_TestPingPong2xMtuWithNaglingOff);
    CU_add_test(pSuite, "PseudoTcpTest", PseudoTcpTestPingPong_TestPingPongShortSegments);
    CU_add_test(pSuite, "PseudoTcpTest", PseudoTcpTestPingPong_TestPingPongShortSegmentsWithNaglingOff);
    CU_add_test(pSuite, "PseudoTcpTest", PseudoTcpTestPingPong_TestPingPongShortSegmentsWithAckDelayOff);
    
#else

    pSuite = CU_add_suite("Test case need to be fixed...", testInit, testClean);
    
    // Test Receive Window
//    CU_add_test(pSuite, "PseudoTcpTest", PseudoTcpTestReceiveWindow_TestReceiveWindow);
//    CU_add_test(pSuite, "PseudoTcpTest", PseudoTcpTestReceiveWindow_TestSetVerySmallSendWindowSize);
//    CU_add_test(pSuite, "PseudoTcpTest", PseudoTcpTestReceiveWindow_TestSetReceiveWindowSize);
 
#endif


    //CU_console_run_tests();
    CU_basic_run_tests();
    CU_cleanup_registry();
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