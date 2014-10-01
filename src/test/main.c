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
    pSuite = CU_add_suite("PseudoTcpTest", testInit, testClean);
    CU_add_test(pSuite, "TestSend", PseudoTcpTest_TestSend);
    CU_add_test(pSuite, "TestSendWithDelay", PseudoTcpTest_TestSendWithDelay);
    CU_add_test(pSuite, "TestSendWithLoss", PseudoTcpTest_TestSendWithLoss);
    CU_add_test(pSuite, "TestSendWithDelayAndLoss", PseudoTcpTest_TestSendWithDelayAndLoss);
    CU_add_test(pSuite, "TestSendWithLossAndOptNaglingOff", PseudoTcpTest_TestSendWithLossAndOptNaglingOff);
    CU_add_test(pSuite, "TestSendWithLossAndOptAckDelayOff", PseudoTcpTest_TestSendWithLossAndOptAckDelayOff);
    CU_add_test(pSuite, "TestSendWithDelayAndOptNaglingOff", PseudoTcpTest_TestSendWithDelayAndOptNaglingOff);
    CU_add_test(pSuite, "TestSendWithDelayAndOptAckDelayOff", PseudoTcpTest_TestSendWithDelayAndOptAckDelayOff);
    CU_add_test(pSuite, "TestSendRemoteNoWindowScale", PseudoTcpTest_TestSendRemoteNoWindowScale);
    CU_add_test(pSuite, "TestSendLocalNoWindowScale", PseudoTcpTest_TestSendLocalNoWindowScale);
    CU_add_test(pSuite, "TestSendBothUseWindowScale", PseudoTcpTest_TestSendBothUseWindowScale);
    CU_add_test(pSuite, "TestSendLargeInFlight", PseudoTcpTest_TestSendLargeInFlight);
    CU_add_test(pSuite, "TestSendBothUseLargeWindowScale", PseudoTcpTest_TestSendBothUseLargeWindowScale);
    CU_add_test(pSuite, "TestSendSmallReceiveBuffer", PseudoTcpTest_TestSendSmallReceiveBuffer);
    CU_add_test(pSuite, "TestSendVerySmallReceiveBuffer", PseudoTcpTest_TestSendVerySmallReceiveBuffer);
    
    // Ping-pong (request/response) tests
    pSuite = CU_add_suite("PseudoTcpTestPingPong", testInit, testClean);
    CU_add_test(pSuite, "TestPingPong1xMtu", PseudoTcpTestPingPong_TestPingPong1xMtu);
    CU_add_test(pSuite, "TestPingPong3xMtu", PseudoTcpTestPingPong_TestPingPong3xMtu);
    CU_add_test(pSuite, "TestPingPong2xMtu", PseudoTcpTestPingPong_TestPingPong2xMtu);
    CU_add_test(pSuite, "TestPingPong2xMtuWithAckDelayOff", PseudoTcpTestPingPong_TestPingPong2xMtuWithAckDelayOff);
    CU_add_test(pSuite, "TestPingPong2xMtuWithNaglingOff", PseudoTcpTestPingPong_TestPingPong2xMtuWithNaglingOff);
    CU_add_test(pSuite, "TestPingPongShortSegments", PseudoTcpTestPingPong_TestPingPongShortSegments);
    CU_add_test(pSuite, "TestPingPongShortSegmentsWithNaglingOff", PseudoTcpTestPingPong_TestPingPongShortSegmentsWithNaglingOff);
    CU_add_test(pSuite, "TestPingPongShortSegmentsWithAckDelayOff", PseudoTcpTestPingPong_TestPingPongShortSegmentsWithAckDelayOff);
    
    // Test Receive Window
    pSuite = CU_add_suite("PseudoTcpTestReceiveWindow", testInit, testClean);
    CU_add_test(pSuite, "TestReceiveWindow", PseudoTcpTestReceiveWindow_TestReceiveWindow);
    CU_add_test(pSuite, "TestSetVerySmallSendWindowSize", PseudoTcpTestReceiveWindow_TestSetVerySmallSendWindowSize);
    CU_add_test(pSuite, "TestSetReceiveWindowSize", PseudoTcpTestReceiveWindow_TestSetReceiveWindowSize);


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
