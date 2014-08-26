/**
 * @file    segment_list.h
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

#ifndef __PSEUDO_TCP_UNITTEST_H__
#define __PSEUDO_TCP_UNITTEST_H__

#ifdef __cplusplus
extern "C" {
#endif

/*** STANDARD INCLUDES *******************************************************/


/*** PROJECT INCLUDES ********************************************************/


/*** MACROS ******************************************************************/


/*** GLOBAL TYPES DEFINITIONS ************************************************/


/*** PRIVATE TYPES DEFINITIONS ***********************************************/


/*** GLOBAL VARIABLE DECLARATIONS (EXTERN) ***********************************/


/*** PUBLIC FUNCTION PROTOTYPES **********************************************/
extern void PseudoTcpTest_01_01(void);

// Basic end-to-end data transfer tests
extern void PseudoTcpTest_TestSend(void);
extern void PseudoTcpTest_TestSendWithDelay(void);
extern void PseudoTcpTest_TestSendWithLoss(void);
extern void PseudoTcpTest_TestSendWithDelayAndLoss(void);
extern void PseudoTcpTest_TestSendWithLossAndOptNaglingOff(void);
extern void PseudoTcpTest_TestSendWithLossAndOptAckDelayOff(void);
extern void PseudoTcpTest_TestSendWithDelayAndOptNaglingOff(void);
extern void PseudoTcpTest_TestSendWithDelayAndOptAckDelayOff(void);
extern void PseudoTcpTest_TestSendRemoteNoWindowScale(void);
extern void PseudoTcpTest_TestSendLocalNoWindowScale(void);
extern void PseudoTcpTest_TestSendBothUseWindowScale(void);
extern void PseudoTcpTest_TestSendLargeInFlight(void);
extern void PseudoTcpTest_TestSendBothUseLargeWindowScale(void);
extern void PseudoTcpTest_TestSendSmallReceiveBuffer(void);
extern void PseudoTcpTest_TestSendVerySmallReceiveBuffer(void);

// Ping-pong (request/response) tests
extern void PseudoTcpTestPingPong_TestPingPong1xMtu(void);
extern void PseudoTcpTestPingPong_TestPingPong3xMtu(void);
extern void PseudoTcpTestPingPong_TestPingPong2xMtu(void);
extern void PseudoTcpTestPingPong_TestPingPong2xMtuWithAckDelayOff(void);
extern void PseudoTcpTestPingPong_TestPingPong2xMtuWithNaglingOff(void);
extern void PseudoTcpTestPingPong_TestPingPongShortSegments(void);
extern void PseudoTcpTestPingPong_TestPingPongShortSegmentsWithNaglingOff(void);
extern void PseudoTcpTestPingPong_TestPingPongShortSegmentsWithAckDelayOff(void);

// Test Receive Window
extern void PseudoTcpTestReceiveWindow_TestReceiveWindow(void);
extern void PseudoTcpTestReceiveWindow_TestSetVerySmallSendWindowSize(void);
extern void PseudoTcpTestReceiveWindow_TestSetReceiveWindowSize(void);
    

/** Doubly Linked List ******************************************************/


/*****************************************************************************/

#ifdef __cplusplus
}
#endif

#endif /* End of #ifndef __PSEUDO_TCP_UNITTEST_H__ */

/******************************************************************************
 *  CONFIDENTIAL
 *****************************************************************************/
