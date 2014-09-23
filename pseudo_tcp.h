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

#ifndef __PSEUDO_TCP_H__
#define __PSEUDO_TCP_H__

#ifdef __cplusplus
extern "C" {
#endif

/*** STANDARD INCLUDES *******************************************************/
#ifdef linux
// MacOS don't have error.h
#include <error.h>
#endif

/*** PROJECT INCLUDES ********************************************************/
#include "pseudo_tcp_porting.h"
#include "fifo_buffer.h"
#include "segment_list.h"

/*** MACROS ******************************************************************/
    
// For Debug only
#define PTCP_LOCAL  0
#define PTCP_REMOTE 1
#define PTCP_STR_LOCAL  "LOCAL"
#define PTCP_STR_REMOTE "REMOTE"
#define PTCP_WHOAMI(x) ({x==PTCP_REMOTE ?  PTCP_STR_REMOTE: PTCP_STR_LOCAL;})

    
/*** GLOBAL TYPES DEFINITIONS ************************************************/
typedef enum {
    TCP_LISTEN, TCP_SYN_SENT, TCP_SYN_RECEIVED, TCP_ESTABLISHED, TCP_CLOSED
} TcpState ;

typedef enum {
    OPT_NODELAY,      // Whether to enable Nagle's algorithm (0 == off)
    OPT_ACKDELAY,     // The Delayed ACK timeout (0 == off).
    OPT_RCVBUF,       // Set the receive buffer size, in bytes.
    OPT_SNDBUF,       // Set the send buffer size, in bytes.
} Option;

typedef enum tWriteResult {
    WR_SUCCESS, WR_TOO_LARGE, WR_FAIL
} tWriteResult;


typedef void  (* tOnTcpOpenCB) (void *) ;
typedef void  (* tOnTcpReadableCB) (void *) ;
typedef void  (* tOnTcpWriteableCB) (void *) ;
typedef void  (* tOnTcpClosedCB) (void *, U32) ;
typedef tWriteResult  (* tTcpWritePacketCB) (void *, U8 *, size_t) ;

typedef struct tIPseudoTcpNotify {
    tOnTcpOpenCB         OnTcpOpen;
    tOnTcpReadableCB     OnTcpReadable;
    tOnTcpWriteableCB    OnTcpWriteable;
    tOnTcpClosedCB       OnTcpClosed;
    tTcpWritePacketCB    TcpWritePacket;
} tIPseudoTcpNotify;


/*** PRIVATE TYPES DEFINITIONS ***********************************************/


/*** GLOBAL VARIABLE DECLARATIONS (EXTERN) ***********************************/
typedef enum  { sfNone, sfDelayedAck, sfImmediateAck } SendFlags;
typedef enum  { SD_NONE, SD_GRACEFUL, SD_FORCEFUL } tShutdown;
typedef struct tSegment {
    U32 conv, seq, ack;
    U8 flags;
    U16 wnd;
    const U8 * data;
    U32 len;
    U32 tsval, tsecr;
} tSegment;


typedef struct tPseudoTcp {
    int m_error;

    // TCB data
    TcpState m_state;
    U32 m_conv;
    bool m_bReadEnable, m_bWriteEnable, m_bOutgoing;
    U32 m_lasttraffic;

    // Incoming data
    tMI_DLIST *m_rlist;
    U32 m_rbuf_len, m_rcv_nxt, m_rcv_wnd, m_lastrecv;
    U8 m_rwnd_scale;  // Window scale factor.
    tFIFO_BUFFER * m_rbuf;

    // Outgoing data
    tMI_DLIST *m_slist;
    U32 m_sbuf_len, m_snd_nxt, m_snd_wnd, m_lastsend, m_snd_una;
    U8 m_swnd_scale;  // Window scale factor.
    tFIFO_BUFFER * m_sbuf;

    // Maximum segment size, estimated protocol level, largest segment sent
    U32 m_mss, m_msslevel, m_largest, m_mtu_advise;
    // Retransmit timer
    U32 m_rto_base;

    // Timestamp tracking
    U32 m_ts_recent, m_ts_lastack;

    // Round-trip calculation
    U32 m_rx_rttvar, m_rx_srtt, m_rx_rto;

    // Congestion avoidance, Fast retransmit/recovery, Delayed ACKs
    U32 m_ssthresh, m_cwnd;
    U8 m_dup_acks;
    U32 m_recover;
    U32 m_t_ack;

    // Configuration options
    bool m_use_nagling;
    U32 m_ack_delay;

    // This is used by unit tests to test backward compatibility of
    // PseudoTcp implementations that don't support window scaling.
    bool m_support_wnd_scale;

    tShutdown m_shutdown;

    tOnTcpOpenCB         OnTcpOpen;
    tOnTcpReadableCB     OnTcpReadable;
    tOnTcpWriteableCB    OnTcpWriteable;
    tOnTcpClosedCB       OnTcpClosed;
    tTcpWritePacketCB    TcpWritePacket;

    // Test only
    U8 tcpId;
} tPseudoTcp;

/*** PUBLIC FUNCTION PROTOTYPES **********************************************/
tPseudoTcp * PTCP_Init(tIPseudoTcpNotify* notify, tFIFO_CB pNotifyReadCB, tFIFO_CB pNotifyWriteCB, U32 conv);
void PTCP_Destroy(tPseudoTcp *pPTCP);

void PTCP_GetOption(tPseudoTcp *pPTCP, Option opt, int* value);
void PTCP_SetOption(tPseudoTcp *pPTCP, Option opt, int value);

int PTCP_Connect(tPseudoTcp *pPTCP);
int PTCP_Recv(tPseudoTcp *pPTCP, U8* buffer, size_t len);
int PTCP_Send(tPseudoTcp *pPTCP, const U8* buffer, size_t len);
void PTCP_Close(tPseudoTcp *pPTCP, bool force);
int PTCP_GetError(tPseudoTcp *pPTCP);
U32 PTCP_Now();

// Call this when the PMTU changes.
void PTCP_NotifyMTU(tPseudoTcp *pPTCP, U16 mtu);

// Call this based on timeout value returned from GetNextClock.
// It's ok to call this too frequently.
void PTCP_NotifyClock(tPseudoTcp *pPTCP, U32 now);

// Call this whenever a packet arrives.
// Returns true if the packet was processed successfully.
bool PTCP_NotifyPacket(tPseudoTcp *pPTCP, const U8 * buffer, size_t len);

// Call this to determine the next time NotifyClock should be called.
// Returns false if the socket is ready to be destroyed.
bool PTCP_GetNextClock(tPseudoTcp *pPTCP, U32 now, long *timeout);
bool PTCP_IsReceiveBufferFull(tPseudoTcp *pPTCP);
void PTCP_DisableWindowScale(tPseudoTcp *pPTCP);

U32 PTCP_GetCongestionWindow(tPseudoTcp *pPTCP);
U32 PTCP_GetBytesInFlight(tPseudoTcp *pPTCP);
U32 PTCP_GetBytesBufferedNotSent(tPseudoTcp *pPTCP);
U32 PTCP_GetRoundTripTimeEstimateMs(tPseudoTcp *pPTCP);

/** Doubly Linked List ******************************************************/


/*****************************************************************************/

#ifdef __cplusplus
}
#endif

#endif /* End of #ifndef __PSEUDO_TCP_H__ */

/******************************************************************************
 *  CONFIDENTIAL
 *****************************************************************************/
