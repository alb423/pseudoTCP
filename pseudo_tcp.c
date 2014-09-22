/**
 * @file    pseudo_tcp.c
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
#include "mi_types.h"
#include "mi_util.h"
#include "byteorder.h"
#include "pseudo_tcp.h"
#include "pseudo_tcp_porting.h"

/*** MACROS ******************************************************************/

// The following logging is for detailed (packet(-level) analysis only.
#define _DBG_NONE     0
#define _DBG_NORMAL   1
#define _DBG_VERBOSE  2
//#define _DEBUGMSG _DBG_NONE
#define _DEBUGMSG _DBG_VERBOSE

// This is used to avoid warning message "unused variable"
#ifndef UNUSED
static inline void Unused(const void *ptr) {}
//void Unused(const void *ptr) {}
#define UNUSED(x) Unused((const void*)(&x))
#endif  // UNUSED

#define INVALID_SOCKET (-1)
#define SOCKET_ERROR (-1)

/*** GLOBAL VARIABLE DECLARATIONS (EXTERN) ***********************************/


/*** PRIVATE TYPES DEFINITIONS ***********************************************/


/*** PRIVATE VARIABLE DECLARATIONS (STATIC) **********************************/


//////////////////////////////////////////////////////////////////////
// Network Constants
//////////////////////////////////////////////////////////////////////

// Standard MTUs
const U16 PACKET_MAXIMUMS[] = {
    65535,    // Theoretical maximum, Hyperchannel
    32000,    // Nothing
    17914,    // 16Mb IBM Token Ring
    8166,   // IEEE 802.4
    //4464,   // IEEE 802.5 (4Mb max)
    4352,   // FDDI
    //2048,   // Wideband Network
    2002,   // IEEE 802.5 (4Mb recommended)
    //1536,   // Expermental Ethernet Networks
    //1500,   // Ethernet, Point-to-Point (default)
    1492,   // IEEE 802.3
    1006,   // SLIP, ARPANET
    //576,    // X.25 Networks
    //544,    // DEC IP Portal
    //512,    // NETBIOS
    508,    // IEEE 802/Source-Rt Bridge, ARCNET
    296,    // Point-to-Point (low delay)
    //68,     // Official minimum
    0,      // End of list marker
};

const U32 MAX_PACKET = 65535;
// Note: we removed lowest level because packet() overhead was larger!
const U32 MIN_PACKET = 296;

const U32 IP_HEADER_SIZE = 20; // (+ up to 40 bytes of options?)
const U32 UDP_HEADER_SIZE = 8;
const U32 UDPTUNNEL_HEADER_SIZE = 5;
// Default size for receive and send buffer.
const U32 DEFAULT_RCV_BUF_SIZE = 60 * 1024;
const U32 DEFAULT_SND_BUF_SIZE = 90 * 1024;

//////////////////////////////////////////////////////////////////////
// Global Constants and Functions
//////////////////////////////////////////////////////////////////////
//
//    0                   1                   2                   3
//    0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
//    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//  0 |                      Conversation Number                      |
//    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//  4 |                        Sequence Number                        |
//    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//  8 |                     Acknowledgment Number                     |
//    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//    |               |   |U|A|P|R|S|F|                               |
// 12 |    Control    |   |R|C|S|S|Y|I|            Window             |
//    |               |   |G|K|H|T|N|N|                               |
//    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
// 16 |                       Timestamp sending                       |
//    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
// 20 |                      Timestamp receiving                      |
//    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
// 24 |                             data                              |
//    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//
//////////////////////////////////////////////////////////////////////

#define PSEUDO_KEEPALIVE 0

const U32 HEADER_SIZE = 24;
// HEADER_SIZE + UDP_HEADER_SIZE + IP_HEADER_SIZE + UDPTUNNEL_HEADER_SIZE;

// TODO: fix me
//const U32 PACKET_OVERHEAD = 24 + 8 + 20 + 5;
// The definition of libjingle
const U32 PACKET_OVERHEAD = 24 + 8 + 20 + 64;
    
const U32 MIN_RTO   =   250; // 250 ms (RFC1122, Sec 4.2.3.1 "fractions of a second")
const U32 DEF_RTO   =  3000; // 3 seconds (RFC1122, Sec 4.2.3.1)
const U32 MAX_RTO   = 60000; // 60 seconds
const U32 DEF_ACK_DELAY = 100; // 100 milliseconds

const U8 FLAG_CTL = 0x02;
const U8 FLAG_RST = 0x04;

const U8 CTL_CONNECT = 0;

// TCP options.
const U8 TCP_OPT_EOL = 0;  // End of list.
const U8 TCP_OPT_NOOP = 1;  // No-op.
const U8 TCP_OPT_MSS = 2;  // Maximum segment size.
const U8 TCP_OPT_WND_SCALE = 3;  // Window scale factor.

const long DEFAULT_TIMEOUT = 4000; // If there are no pending clocks, wake up every 4 seconds
const long CLOSED_TIMEOUT = 60 * 1000; // If the connection is closed, once per minute

#if PSEUDO_KEEPALIVE
// !?! Rethink these times
const U32 IDLE_PING = 20 * 1000; // 20 seconds (note: WinXP SP2 firewall udp timeout is 90 seconds)
const U32 IDLE_TIMEOUT = 90 * 1000; // 90 seconds;
#endif // PSEUDO_KEEPALIVE

/*** PRIVATE FUNCTION PROTOTYPES *********************************************/
void queueConnectMessage(tPseudoTcp *pPTCP);
void attemptSend(tPseudoTcp *pPTCP, SendFlags sflags);
void adjustMTU(tPseudoTcp *pPTCP);
bool transmit(tPseudoTcp *pPTCP, tRSSegment *seg, U32 now);
void closedown(tPseudoTcp *pPTCP, U32 err);
bool clock_check(tPseudoTcp *pPTCP, U32 now, long  *pTimeout);
void parseOptions(tPseudoTcp *pPTCP, const U8* data, U32 len);
void resizeSendBuffer(tPseudoTcp *pPTCP, U32 new_size);
void resizeReceiveBuffer(tPseudoTcp *pPTCP, U32 new_size);
void applyOption(tPseudoTcp *pPTCP, U8 kind, const U8* data, U32 len);
void applyWindowScaleOption(tPseudoTcp *pPTCP, U8 scale_factor);

U32 PTCP_Queue(tPseudoTcp *pPTCP, const U8* data, U32 len, bool bCtrl);
tWriteResult PTCP_Packet(tPseudoTcp *pPTCP, U32 seq, U8 flags, U32 offset, U32 len);
bool PTCP_Parse(tPseudoTcp *pPTCP, const U8* buffer, U32 size);
bool PTCP_Process(tPseudoTcp *pPTCP, tSegment *pSeg);
void PTCP_GetOption(tPseudoTcp *pPTCP, Option opt, int* value);
void PTCP_SetOption(tPseudoTcp *pPTCP, Option opt, int value);
U32 PTCP_GetCongestionWindow(tPseudoTcp *pPTCP);
U32 PTCP_GetBytesInFlight(tPseudoTcp *pPTCP);
U32 PTCP_GetBytesBufferedNotSent(tPseudoTcp *pPTCP);
U32 PTCP_GetRoundTripTimeEstimateMs(tPseudoTcp *pPTCP);

//////////////////////////////////////////////////////////////////////
// Helper Functions
//////////////////////////////////////////////////////////////////////

static inline void long_to_bytes(U32 val, void* buf)
{
    *(U32 *)(buf) = HostToNetwork32(val);
}

static inline void short_to_bytes(U16 val, void* buf)
{
    *(U16 *)(buf) = HostToNetwork16(val);
}

static inline U32 bytes_to_long(const void* buf)
{
    return NetworkToHost32(*(U32 *)(buf));
}

static inline U16 bytes_to_short(const void* buf)
{
    return NetworkToHost16(*(U16 *)(buf));
}

U32 bound(U32 lower, U32 middle, U32 upper)
{
    return MIN(MAX(lower, middle), upper);
}

//////////////////////////////////////////////////////////////////////
// PseudoTcp
//////////////////////////////////////////////////////////////////////

U32 PTCP_Now()
{
    return Time();
}


tPseudoTcp * PTCP_Init(tIPseudoTcpNotify* notify, tFIFO_CB *pNotifyReadCB, tFIFO_CB *pNotifyWriteCB, U32 conv)
{

    tPseudoTcp *pPTCP = NULL;

    if(notify==NULL)
        return NULL;

    pPTCP = malloc(sizeof(tPseudoTcp));
    if(!pPTCP)
        return NULL;
    memset(pPTCP, 0, sizeof(tPseudoTcp));

    pPTCP->OnTcpOpen        = notify->OnTcpOpen;
    pPTCP->OnTcpReadable    = notify->OnTcpReadable;
    pPTCP->OnTcpWriteable   = notify->OnTcpWriteable;
    pPTCP->OnTcpClosed      = notify->OnTcpClosed;
    pPTCP->TcpWritePacket   = notify->TcpWritePacket;

    pPTCP->m_shutdown = SD_NONE;
    pPTCP->m_error = 0;

    pPTCP->m_rbuf_len = DEFAULT_RCV_BUF_SIZE;
    pPTCP->m_sbuf_len = DEFAULT_SND_BUF_SIZE;
    pPTCP->m_rbuf = FIFO_Create(pPTCP->m_rbuf_len, pNotifyReadCB, pNotifyWriteCB);
    pPTCP->m_sbuf = FIFO_Create(pPTCP->m_sbuf_len, pNotifyReadCB, pNotifyWriteCB);

    pPTCP->m_rlist = malloc(sizeof(tMI_DLIST));
    pPTCP->m_slist = malloc(sizeof(tMI_DLIST));
    SEGMENT_LIST_Init(pPTCP->m_rlist);
    SEGMENT_LIST_Init(pPTCP->m_slist);

    // Sanity check on buffer sizes (needed for OnTcpWriteable notification logic)
    ASSERT(pPTCP->m_rbuf_len + MIN_PACKET < pPTCP->m_sbuf_len);

    U32 now = PTCP_Now();

    pPTCP->m_state = TCP_LISTEN;
    pPTCP->m_conv = conv;
    pPTCP->m_rcv_wnd = pPTCP->m_rbuf_len;
    pPTCP->m_rwnd_scale = pPTCP->m_swnd_scale = 0;
    pPTCP->m_snd_nxt = 0;
    pPTCP->m_snd_wnd = 1;
    pPTCP->m_snd_una = pPTCP->m_rcv_nxt = 0;
    pPTCP->m_bReadEnable = true;
    pPTCP->m_bWriteEnable = false;
    pPTCP->m_t_ack = 0;

    pPTCP->m_msslevel = 0;
    pPTCP->m_largest = 0;
    ASSERT(MIN_PACKET > PACKET_OVERHEAD);
    pPTCP->m_mss = MIN_PACKET - PACKET_OVERHEAD;
    pPTCP->m_mtu_advise = MAX_PACKET;

    pPTCP->m_rto_base = 0;

    pPTCP->m_cwnd = 2 * pPTCP->m_mss;
    pPTCP->m_ssthresh = pPTCP->m_rbuf_len;
    pPTCP->m_lastrecv = pPTCP->m_lastsend = pPTCP->m_lasttraffic = now;
    pPTCP->m_bOutgoing = false;

    pPTCP->m_dup_acks = 0;
    pPTCP->m_recover = 0;

    pPTCP->m_ts_recent = pPTCP->m_ts_lastack = 0;

    pPTCP->m_rx_rto = DEF_RTO;
    pPTCP->m_rx_srtt = pPTCP->m_rx_rttvar = 0;

    pPTCP->m_use_nagling = true;
    pPTCP->m_ack_delay = DEF_ACK_DELAY;
    pPTCP->m_support_wnd_scale = true;

    return pPTCP;
}

void PTCP_Destroy(tPseudoTcp *pPTCP)
{
    if(pPTCP) {
        if(pPTCP->m_rbuf) {
            FIFO_Destroy(pPTCP->m_rbuf);
        }

        if(pPTCP->m_sbuf) {
            FIFO_Destroy(pPTCP->m_sbuf);
        }

#if 0
        free(pPTCP->m_rlist);
        free(pPTCP->m_slist);
#else
        // If there are data(memory leakage) still in m_rlist or m_slist, we should remove them here
        if(pPTCP->m_rlist) {
            if(pPTCP->m_rlist->count!=0) {
                SEGMENT_LIST_Dump(pPTCP->m_rlist);
            }
            while(pPTCP->m_rlist->count!=0) {
                tRSSegment * vpRSSeg = SEGMENT_LIST_front(pPTCP->m_rlist);
                SEGMENT_LIST_pop_front(pPTCP->m_rlist);
                if(vpRSSeg) {
                    free(vpRSSeg);
                }
            }
            free(pPTCP->m_rlist);
        }
        if(pPTCP->m_slist) {
            if(pPTCP->m_slist->count!=0) {
                SEGMENT_LIST_Dump(pPTCP->m_slist);
                tRSSegment * vpRSSeg = SEGMENT_LIST_front(pPTCP->m_slist);
                printf("==> seq=%d len=%d xmit=%d\n", vpRSSeg->seq, vpRSSeg->len, vpRSSeg->xmit);
            }
            while(pPTCP->m_slist->count!=0) {
                tRSSegment * vpRSSeg = SEGMENT_LIST_front(pPTCP->m_slist);
                if(vpRSSeg) {
                    free(vpRSSeg);
                }
                SEGMENT_LIST_pop_front(pPTCP->m_slist);
            }
            free(pPTCP->m_slist);
        }
#endif
        free(pPTCP);
    }
}

int PTCP_Connect(tPseudoTcp *pPTCP)
{
    ASSERT(pPTCP!=NULL);
    if (pPTCP->m_state != TCP_LISTEN) {
        pPTCP->m_error = EINVAL;
        return -1;
    }

    pPTCP->m_state = TCP_SYN_SENT;
    //LOG_F(LS_INFO, "State: TCP_LISTEN -> TCP_SYN_SENT");
    char pTmp[1024]= {0};
    sprintf(pTmp, "%s: State: TCP_LISTEN -> TCP_SYN_SENT", PTCP_WHOAMI(pPTCP->tcpId));
    LOG_F(LS_VERBOSE, pTmp);
    
    queueConnectMessage(pPTCP);
    attemptSend(pPTCP, sfNone);

    return 0;
}


void PTCP_NotifyMTU(tPseudoTcp *pPTCP, U16 mtu)
{
    ASSERT(pPTCP!=NULL);
    pPTCP->m_mtu_advise = mtu;
    if (pPTCP->m_state == TCP_ESTABLISHED) {
        adjustMTU(pPTCP);
    }
}


void PTCP_NotifyClock(tPseudoTcp *pPTCP, U32 now)
{
    ASSERT(pPTCP!=NULL);
    if (pPTCP->m_state == TCP_CLOSED)
        return;

    // Check if it's time to retransmit a segment
    char pTmp[1024]= {0};
    sprintf(pTmp, "%s: m_rto_base=%d, TimeDiff=%d now=%d m_rto_base=%d, m_rx_rto=%d",\
            PTCP_WHOAMI(pPTCP->tcpId), pPTCP->m_rto_base, TimeDiff(pPTCP->m_rto_base + pPTCP->m_rx_rto, now), now, pPTCP->m_rto_base, pPTCP->m_rx_rto);
    LOG_F(LS_VERBOSE, pTmp);

    if (pPTCP->m_rto_base && (TimeDiff(pPTCP->m_rto_base + pPTCP->m_rx_rto, now) <= 0)) {
        if (SEGMENT_LIST_empty(pPTCP->m_slist)) {
            ASSERT(false);
        } else {
            // Note: (pPTCP->m_slist.front().xmit == 0)) {
            // retransmit segments
//#if _DEBUGMSG >= _DBG_NORMAL
#if 1
            char pTmp[1024]= {0};
            sprintf(pTmp, "%s: timeout retransmit (rto: %d) (rto_base: %d) (now: %d) (dup_acks: %d)",\
                    PTCP_WHOAMI(pPTCP->tcpId), pPTCP->m_rx_rto, pPTCP->m_rto_base, now, (unsigned)pPTCP->m_dup_acks);
            LOG_F(LS_VERBOSE, pTmp);
#endif // _DEBUGMSG
            
            if (!transmit(pPTCP, SEGMENT_LIST_begin(pPTCP->m_slist), now)) {
                closedown(pPTCP, ECONNABORTED);
                return;
            }

            U32 nInFlight = pPTCP->m_snd_nxt - pPTCP->m_snd_una;
            pPTCP->m_ssthresh = MAX(nInFlight / 2, 2 * pPTCP->m_mss);
            //LOG_F(LS_INFO) << "pPTCP->m_ssthresh: " << pPTCP->m_ssthresh << "  nInFlight: " << nInFlight << "  pPTCP->m_mss: " << pPTCP->m_mss;
            pPTCP->m_cwnd = pPTCP->m_mss;

            // Back off retransmit timer.  Note: the limit is lower when connecting.
            U32 rto_limit = (pPTCP->m_state < TCP_ESTABLISHED) ? DEF_RTO : MAX_RTO;
            pPTCP->m_rx_rto = MIN(rto_limit, pPTCP->m_rx_rto * 2);
            pPTCP->m_rto_base = now;
        }
    }

    // Check if it's time to probe closed windows
    if ((pPTCP->m_snd_wnd == 0)
            && (TimeDiff(pPTCP->m_lastsend + pPTCP->m_rx_rto, now) <= 0)) {
        if (TimeDiff(now, pPTCP->m_lastrecv) >= 15000) {
            closedown(pPTCP, ECONNABORTED);
            return;
        }

        // probe the window
        PTCP_Packet(pPTCP, pPTCP->m_snd_nxt - 1, 0, 0, 0);
        pPTCP->m_lastsend = now;

        // back off retransmit timer
        pPTCP->m_rx_rto = MIN(MAX_RTO, pPTCP->m_rx_rto * 2);
    }

    // Check if it's time to send delayed acks
    if (pPTCP->m_t_ack && (TimeDiff(pPTCP->m_t_ack + pPTCP->m_ack_delay, now) <= 0)) {
        PTCP_Packet(pPTCP, pPTCP->m_snd_nxt, 0, 0, 0);
    }

#if PSEUDO_KEEPALIVE
    // Check for idle timeout
    if ((pPTCP->m_state == TCP_ESTABLISHED) && (TimeDiff(pPTCP->m_lastrecv + IDLE_TIMEOUT, now) <= 0)) {
        closedown(pPTCP, ECONNABORTED);
        return;
    }

    // Check for ping timeout (to keep udp mapping open)
    if ((pPTCP->m_state == TCP_ESTABLISHED) && (TimeDiff(pPTCP->m_lasttraffic + (pPTCP->m_bOutgoing ? IDLE_PING * 3/2 : IDLE_PING), now) <= 0)) {
        PTCP_Packet(pPTCP, pPTCP->m_snd_nxt, 0, 0, 0);
    }
#endif // PSEUDO_KEEPALIVE
}


bool PTCP_NotifyPacket(tPseudoTcp *pPTCP, const U8* buffer, size_t len)
{
    char pTmp[1024]= {0};
    sprintf(pTmp, "%s: PTCP_NotifyPacket In", PTCP_WHOAMI(pPTCP->tcpId));
    LOG_F(LS_VERBOSE, pTmp);
    
    //LOG_F(LS_INFO, "PTCP_NotifyPacket In");
    ASSERT(pPTCP!=NULL);
    if (len > MAX_PACKET) {
        LOG_F(LS_INFO, "PTCP_Packet too large");
        return false;
    }
    //LOG_F(LS_INFO, "PTCP_NotifyPacket Out");
    return PTCP_Parse(pPTCP, (const U8 *)buffer, (U32)len);
}


bool PTCP_GetNextClock(tPseudoTcp *pPTCP, U32 now, long *pTimeout)
{
    return clock_check(pPTCP, now, pTimeout);
}


void PTCP_GetOption(tPseudoTcp *pPTCP, Option opt, int* value)
{
    ASSERT(pPTCP!=NULL);
    if (opt == OPT_NODELAY) {
        *value = pPTCP->m_use_nagling ? 0 : 1;
    } else if (opt == OPT_ACKDELAY) {
        *value = pPTCP->m_ack_delay;
    } else if (opt == OPT_SNDBUF) {
        *value = pPTCP->m_sbuf_len;
    } else if (opt == OPT_RCVBUF) {
        *value = pPTCP->m_rbuf_len;
    } else {
        ASSERT(false);
    }
}


void PTCP_SetOption(tPseudoTcp *pPTCP, Option opt, int value)
{
    ASSERT(pPTCP!=NULL);
    if (opt == OPT_NODELAY) {
        pPTCP->m_use_nagling = value == 0;
    } else if (opt == OPT_ACKDELAY) {
        pPTCP->m_ack_delay = value;
    } else if (opt == OPT_SNDBUF) {
        ASSERT(pPTCP->m_state == TCP_LISTEN);
        resizeSendBuffer(pPTCP, value);
    } else if (opt == OPT_RCVBUF) {
        ASSERT(pPTCP->m_state == TCP_LISTEN);
        resizeReceiveBuffer(pPTCP, value);
    } else {
        ASSERT(false);
    }
}


U32 PTCP_GetCongestionWindow(tPseudoTcp *pPTCP)
{
    ASSERT(pPTCP!=NULL);
    return pPTCP->m_cwnd;
}


U32 PTCP_GetBytesInFlight(tPseudoTcp *pPTCP)
{
    ASSERT(pPTCP!=NULL);
    return pPTCP->m_snd_nxt - pPTCP->m_snd_una;
}


U32 PTCP_GetBytesBufferedNotSent(tPseudoTcp *pPTCP)
{
    ASSERT(pPTCP!=NULL);
    size_t buffered_bytes = 0;
    FIFO_GetBuffered(pPTCP->m_sbuf, &buffered_bytes);
    return (U32)(pPTCP->m_snd_una + buffered_bytes - pPTCP->m_snd_nxt);
}


U32 PTCP_GetRoundTripTimeEstimateMs(tPseudoTcp *pPTCP)
{
    ASSERT(pPTCP!=NULL);
    return pPTCP->m_rx_srtt;
}


//
// IPStream Implementation
//
//-->


int PTCP_Recv(tPseudoTcp *pPTCP, U8* buffer, size_t len)
{
    ASSERT(pPTCP!=NULL);
    if (pPTCP->m_state != TCP_ESTABLISHED) {
        pPTCP->m_error = ENOTCONN;
        return SOCKET_ERROR;
    }

    size_t read = 0;
    tStreamResult result = FIFO_Read(pPTCP->m_rbuf, buffer, len, &read);

    // If there's no data in |pPTCP->m_rbuf|.
    if (result == SR_BLOCK) {
        pPTCP->m_bReadEnable = true;
        pPTCP->m_error = EWOULDBLOCK;
        return SOCKET_ERROR;
    }
    ASSERT(result == SR_SUCCESS);

#ifdef CHECK_COPY_BUFFER
    size_t i=0;
    if(read > 24+4) {
        i = read-1;
        if(((char *)buffer)[i]==0x00) {
            ASSERT(((char *)buffer)[i]!=0x00);
        }
    }
#endif

    size_t available_space = 0;
    FIFO_GetWriteRemaining(pPTCP->m_rbuf, &available_space);

    if ((U32)(available_space) - pPTCP->m_rcv_wnd >= (U32)(MIN(pPTCP->m_rbuf_len / 2, pPTCP->m_mss))) {
        // TODO(jbeda): !?! Not sure about this was closed business
        bool bWasClosed = (pPTCP->m_rcv_wnd == 0);
        pPTCP->m_rcv_wnd = (U32)(available_space);

        if (bWasClosed) {
            LOG_F(LS_INFO, "bWasClosed");
            attemptSend(pPTCP, sfImmediateAck);
        }
    }

    return (S32)(read);
}


int PTCP_Send(tPseudoTcp *pPTCP, const U8* buffer, size_t len)
{
    ASSERT(pPTCP!=NULL);
    if (pPTCP->m_state != TCP_ESTABLISHED) {
        pPTCP->m_error = ENOTCONN;
        return SOCKET_ERROR;
    }

    size_t available_space = 0;
    FIFO_GetWriteRemaining(pPTCP->m_sbuf, &available_space);

    if (!available_space) {
        pPTCP->m_bWriteEnable = true;
        pPTCP->m_error = EWOULDBLOCK;
        return SOCKET_ERROR;
    }

    int written = PTCP_Queue(pPTCP, buffer, (U32)len, false);
    //LOG_F(LS_INFO, "PTCP_Send");
    char pTmp[1024]= {0};
    sprintf(pTmp, "%s: PTCP_Send", PTCP_WHOAMI(pPTCP->tcpId));
    LOG_F(LS_VERBOSE, pTmp);
    
    attemptSend(pPTCP, sfNone);
    return written;
}

void PTCP_Close(tPseudoTcp *pPTCP, bool force)
{
    LOG_ARG(LS_VERBOSE, force);
    pPTCP->m_shutdown = force ? SD_FORCEFUL : SD_GRACEFUL;
}

int PTCP_GetError(tPseudoTcp *pPTCP)
{
    ASSERT(pPTCP!=NULL);
    return pPTCP->m_error;
}


U32 PTCP_Queue(tPseudoTcp *pPTCP, const U8* data, U32 len, bool bCtrl)
{
    size_t available_space = 0;
    ASSERT(pPTCP!=NULL);
    FIFO_GetWriteRemaining(pPTCP->m_sbuf, &available_space);

    if (len > (U32)(available_space)) {
        ASSERT(!bCtrl);
        len = (U32)(available_space);
    }
    
    // We can concatenate data if the last segment is the same type
    // (control v. regular data), and has not been transmitted yet
    if (!SEGMENT_LIST_empty(pPTCP->m_slist) && ((SEGMENT_LIST_back(pPTCP->m_slist))->bCtrl == bCtrl) &&
            ((SEGMENT_LIST_back(pPTCP->m_slist))->xmit == 0)) {
        (SEGMENT_LIST_back(pPTCP->m_slist))->len +=len;

    } else {
        size_t snd_buffered = 0;
        FIFO_GetBuffered(pPTCP->m_sbuf, &snd_buffered);
        //SSegment sseg(static_cast<uint32>(pPTCP->m_snd_una + snd_buffered), len, bCtrl);
        tRSSegment *pRSSeg = malloc(sizeof(tRSSegment));
        memset(pRSSeg, 0, sizeof(tRSSegment));
        pRSSeg->seq = (U32)(pPTCP->m_snd_una + snd_buffered);
        pRSSeg->len = len;
        pRSSeg->bCtrl = bCtrl;
        SEGMENT_LIST_push_back(pPTCP->m_slist, &(pRSSeg->DLNode));
        //printf("PTCP_Queue &(pRSSeg->DLNode)=0x%08x\n", (U32)&(pRSSeg->DLNode));
    }
    size_t written = 0;
    tStreamResult result = FIFO_Write(pPTCP->m_sbuf, data, len, &written);
    ASSERT(result == SR_SUCCESS);
    return (U32)(written);
}


tWriteResult PTCP_Packet(tPseudoTcp *pPTCP, U32 seq, U8 flags, U32 offset, U32 len)
{
    char pTmp[1024]= {0};
    sprintf(pTmp, "%s: PTCP_Packet size(%d)", PTCP_WHOAMI(pPTCP->tcpId), len);
    LOG_F(LS_VERBOSE, pTmp);
    //LOG_F(LS_INFO, "PTCP_Packet");
    ASSERT(pPTCP!=NULL);
    ASSERT(HEADER_SIZE + len <= MAX_PACKET);
    U32 now = PTCP_Now();

    // talk_base::scoped_ptr<uint8[]> buffer(new uint8[MAX_PACKET]);
//   U8 *buffer = malloc(MAX_PACKET);
//   if(!buffer)
//      return WR_FAIL;
//   memset(buffer, 0, MAX_PACKET);

    U8 buffer[MAX_PACKET] = {0};
    long_to_bytes(pPTCP->m_conv, &buffer[0]);
    long_to_bytes(seq, &buffer[4]);
    long_to_bytes(pPTCP->m_rcv_nxt, &buffer[8]);
    buffer[12] = 0;
    buffer[13] = flags;
    short_to_bytes((U16)(pPTCP->m_rcv_wnd >> pPTCP->m_rwnd_scale), &buffer[14]);

    // Timestamp computations
    long_to_bytes(now, &buffer[16]);
    long_to_bytes(pPTCP->m_ts_recent, &buffer[20]);
    pPTCP->m_ts_lastack = pPTCP->m_rcv_nxt;

    if (len) {
        size_t bytes_read = 0;
        tStreamResult result = FIFO_ReadOffset(
                                   pPTCP->m_sbuf, &buffer[HEADER_SIZE], len, offset, &bytes_read);

        UNUSED(result);
        ASSERT(result == SR_SUCCESS);
        ASSERT((U32)(bytes_read) == len);
    }

#if _DEBUGMSG >= _DBG_VERBOSE
    char pTmpBuf[1024]= {0};
    sprintf(pTmpBuf, "<-- <CONV=%d><FLG=%d><SEQ=%d><ACK=%d><WND=%d><TS=%d><TSR=%d><LEN=%d>",\
            pPTCP->m_conv, (unsigned)flags, (seq + len), pPTCP->m_rcv_nxt, pPTCP->m_rcv_wnd, (now % 10000), (pPTCP->m_ts_recent % 10000), len);
    LOG(LS_VERBOSE, pTmpBuf);
#endif // _DEBUGMSG

    //WriteResult wres = pPTCP->TcpWritePacket(this, buffer[0], len + HEADER_SIZE);
    tWriteResult wres = pPTCP->TcpWritePacket(pPTCP, &buffer[0], len + HEADER_SIZE);

    // Note: When len is 0, this is an ACK PTCP_Packet.  We don't read the return value for those,
    // and thus we won't retry.  So go ahead and treat the PTCP_Packet as a success (basically simulate
    // as if it were dropped), which will prevent our timers from being messed up.
    if ((wres != WR_SUCCESS) && (0 != len))
        return wres;

    pPTCP->m_t_ack = 0;
    if (len > 0) {
        pPTCP->m_lastsend = now;
    }
    pPTCP->m_lasttraffic = now;
    pPTCP->m_bOutgoing = true;

    return WR_SUCCESS;
}


bool PTCP_Parse(tPseudoTcp *pPTCP, const U8* buffer, U32 size)
{
    ASSERT(pPTCP!=NULL);
    if (size < 12)
        return false;

    tSegment seg;
    seg.conv = bytes_to_long(buffer);
    seg.seq = bytes_to_long(buffer + 4);
    seg.ack = bytes_to_long(buffer + 8);
    seg.flags = buffer[13];
    seg.wnd = bytes_to_short(buffer + 14);

    seg.tsval = bytes_to_long(buffer + 16);
    seg.tsecr = bytes_to_long(buffer + 20);

    seg.data = buffer + HEADER_SIZE;
    seg.len = size - HEADER_SIZE;

#if _DEBUGMSG >= _DBG_VERBOSE
    char pTmp[1024] = {0};
    sprintf(pTmp, "--> <CONV=%d><FLG=%d><SEQ=%d:%d><ACK=%d><WND=%d><TS=%d><TSR=%d><LEN=%d>", \
            seg.conv, (unsigned)seg.flags, seg.seq, seg.seq + seg.len, seg.ack, seg.wnd,\
            (seg.tsval % 10000), (seg.tsecr % 10000), seg.len);
    LOG(LS_INFO, pTmp);
#endif // _DEBUGMSG

    return PTCP_Process(pPTCP, &seg);
}


bool clock_check(tPseudoTcp *pPTCP, U32 now, long  *pTimeout)
{
    ASSERT(pPTCP!=NULL);
    long nTimeout;
    size_t snd_buffered = 0;

    if (pPTCP->m_shutdown == SD_FORCEFUL)
        return false;

    FIFO_GetBuffered(pPTCP->m_sbuf, &snd_buffered);
    if ((pPTCP->m_shutdown == SD_GRACEFUL)
            && ((pPTCP->m_state != TCP_ESTABLISHED)
                || ((snd_buffered == 0) && (pPTCP->m_t_ack == 0)))) {
        return false;
    }

    if (pPTCP->m_state == TCP_CLOSED) {
        nTimeout = CLOSED_TIMEOUT;
        return true;
    }

    nTimeout = DEFAULT_TIMEOUT;

    if (pPTCP->m_t_ack) {
        nTimeout = (S32) MIN(nTimeout, TimeDiff(pPTCP->m_t_ack + pPTCP->m_ack_delay, now));
    }
    if (pPTCP->m_rto_base) {
        nTimeout = (S32) MIN(nTimeout, TimeDiff(pPTCP->m_rto_base + pPTCP->m_rx_rto, now));
    }
    if (pPTCP->m_snd_wnd == 0) {
        nTimeout = (S32) MIN(nTimeout, TimeDiff(pPTCP->m_lastsend + pPTCP->m_rx_rto, now));
    }
#if PSEUDO_KEEPALIVE
    if (pPTCP->m_state == TCP_ESTABLISHED) {
        nTimeout = (S32) MIN(nTimeout,
                             TimeDiff(pPTCP->m_lasttraffic + (pPTCP->m_bOutgoing ? IDLE_PING * 3/2 : IDLE_PING), now));
    }
#endif // PSEUDO_KEEPALIVE
    *pTimeout = nTimeout;
    return true;
}


bool PTCP_Process(tPseudoTcp *pPTCP, tSegment *pSeg)
{
    //LOG_F(LS_INFO, "PTCP_Process");
    ASSERT(pPTCP!=NULL);
    char pTmp[1024]= {0};
    // If this is the wrong conversation, send a reset!?! (with the correct conversation?)
    if (pSeg->conv != pPTCP->m_conv) {
        //if ((seg.flags & FLAG_RST) == 0) {
        //  PTCP_Packet(tcb, seg.ack, 0, FLAG_RST, 0, 0);
        //}
        LOG_F(LS_INFO, "wrong conversation");
        return false;
    }

    U32 now = PTCP_Now();
    pPTCP->m_lasttraffic = pPTCP->m_lastrecv = now;
    pPTCP->m_bOutgoing = false;

    if (pPTCP->m_state == TCP_CLOSED) {
        // !?! send reset?
        LOG_F(LS_INFO, "closed");
        return false;
    }

    // Check if this is a reset segment
    if (pSeg->flags & FLAG_RST) {
        closedown(pPTCP, ECONNRESET);
        return false;
    }

    // Check for control data
    bool bConnect = false;
    if (pSeg->flags & FLAG_CTL) {
        if (pSeg->len == 0) {
            LOG_F(LS_INFO, "Missing control code");
            return false;
        } else if (pSeg->data[0] == CTL_CONNECT) {
            bConnect = true;

            // TCP options are in the remainder of the payload after CTL_CONNECT.
            parseOptions(pPTCP, &pSeg->data[1], pSeg->len - 1);
            
            if (pPTCP->m_state == TCP_LISTEN) {
                pPTCP->m_state = TCP_SYN_RECEIVED;
                //LOG_F(LS_INFO, "State: TCP_LISTEN -> TCP_SYN_RECEIVED");
                char pTmp[1024]= {0};
                sprintf(pTmp, "%s: State: TCP_LISTEN -> TCP_SYN_RECEIVED", PTCP_WHOAMI(pPTCP->tcpId));
                LOG_F(LS_VERBOSE, pTmp);
                //pPTCP->associate(addr);
                queueConnectMessage(pPTCP);
            } else if (pPTCP->m_state == TCP_SYN_SENT) {
                pPTCP->m_state = TCP_ESTABLISHED;
                //LOG_F(LS_INFO, "State: TCP_SYN_SENT -> TCP_ESTABLISHED");
                char pTmp[1024]= {0};
                sprintf(pTmp, "%s: State: TCP_SYN_SENT -> TCP_ESTABLISHED", PTCP_WHOAMI(pPTCP->tcpId));
                LOG_F(LS_VERBOSE, pTmp);
                adjustMTU(pPTCP);
                //pPTCP->OnTcpOpen(this);
                if(pPTCP->OnTcpOpen) {
                    pPTCP->OnTcpOpen(pPTCP);
                }
                //notify(evOpen);
            }
        } else {
            sprintf(pTmp, "Unknown control code: %d", pSeg->data[0]);
            LOG_F(LS_ERROR, pTmp);
            return false;
        }
    }

    // Update timestamp
    if ((pSeg->seq <= pPTCP->m_ts_lastack) && (pPTCP->m_ts_lastack < pSeg->seq + pSeg->len)) {
        pPTCP->m_ts_recent = pSeg->tsval;
    }

    // Check if this is a valuable ack
    if ((pSeg->ack > pPTCP->m_snd_una) && (pSeg->ack <= pPTCP->m_snd_nxt)) {
        // Calculate round-trip time
        if (pSeg->tsecr) {
            S32 rtt = TimeDiff(now, pSeg->tsecr);
            if (rtt >= 0) {
                if (pPTCP->m_rx_srtt == 0) {
                    pPTCP->m_rx_srtt = rtt;
                    pPTCP->m_rx_rttvar = rtt / 2;
                } else {
                    U32 unsigned_rtt = (U32)(rtt);
                    U32 abs_err = unsigned_rtt > pPTCP->m_rx_srtt ? unsigned_rtt - pPTCP->m_rx_srtt
                                  : pPTCP->m_rx_srtt - unsigned_rtt;
                    pPTCP->m_rx_rttvar = (3 * pPTCP->m_rx_rttvar + abs_err) / 4;
                    pPTCP->m_rx_srtt = (7 * pPTCP->m_rx_srtt + rtt) / 8;
                }
                pPTCP->m_rx_rto = bound(MIN_RTO, pPTCP->m_rx_srtt + (U32)MAX(1, 4 * pPTCP->m_rx_rttvar), MAX_RTO);
#if _DEBUGMSG >= _DBG_VERBOSE
                memset(pTmp, 0, 1024);
                sprintf(pTmp, "rtt: %d  srtt: %d  rto: %d\n", rtt, pPTCP->m_rx_srtt, pPTCP->m_rx_rto);
                LOG(LS_VERBOSE, pTmp);
#endif // _DEBUGMSG
            } else {
                ASSERT(false);
            }
        }

        pPTCP->m_snd_wnd = (U32)(pSeg->wnd) << pPTCP->m_swnd_scale;

        U32 nAcked = pSeg->ack - pPTCP->m_snd_una;
        pPTCP->m_snd_una = pSeg->ack;

        pPTCP->m_rto_base = (pPTCP->m_snd_una == pPTCP->m_snd_nxt) ? 0 : now;
        
        //TODO: check here
        if ((!pPTCP->m_rto_base) || (nAcked ==0))
        {
            LOG_F(LS_VERBOSE, "check pPTCP->m_rto_base\n");
        }
            
        //pPTCP->m_sbuf.ConsumeReadData(nAcked);
        FIFO_ConsumeReadData(pPTCP->m_sbuf, nAcked);

        U32 nFree;
        for (nFree = nAcked; nFree > 0; ) {
            ASSERT(!SEGMENT_LIST_empty(pPTCP->m_slist));

            tRSSegment *vpRSSegment = NULL;
            vpRSSegment = SEGMENT_LIST_front(pPTCP->m_slist);
            if(vpRSSegment) {
                if (nFree < vpRSSegment->len) {
                    vpRSSegment->len -= nFree;
                    nFree = 0;
                } else {
                    if (vpRSSegment->len > pPTCP->m_largest) {
                        pPTCP->m_largest = vpRSSegment->len;
                    }
                    nFree -= vpRSSegment->len;
                    SEGMENT_LIST_pop_front(pPTCP->m_slist);
                    free(vpRSSegment);
                }
            } else {
                break;
            }
        }

        if (pPTCP->m_dup_acks >= 3) {
            if (pPTCP->m_snd_una >= pPTCP->m_recover) { // NewReno
                U32 nInFlight = pPTCP->m_snd_nxt - pPTCP->m_snd_una;
                pPTCP->m_cwnd = MIN(pPTCP->m_ssthresh, nInFlight + pPTCP->m_mss); // (Fast Retransmit)
#if _DEBUGMSG >= _DBG_NORMAL
                LOG_F(LS_INFO, "exit recovery");
#endif // _DEBUGMSG
                pPTCP->m_dup_acks = 0;
            } else {
#if _DEBUGMSG >= _DBG_NORMAL
                LOG_F(LS_INFO, "recovery retransmit");
#endif // _DEBUGMSG
                if (!transmit(pPTCP, SEGMENT_LIST_begin(pPTCP->m_slist), now)) {
                    closedown(pPTCP, ECONNABORTED);
                    return false;
                }
                pPTCP->m_cwnd += pPTCP->m_mss - MIN(nAcked, pPTCP->m_cwnd);
            }
        } else {
            pPTCP->m_dup_acks = 0;
            // Slow start, congestion avoidance
            if (pPTCP->m_cwnd < pPTCP->m_ssthresh) {
                pPTCP->m_cwnd += pPTCP->m_mss;
            } else {
                pPTCP->m_cwnd += (U32)MAX(1, pPTCP->m_mss * pPTCP->m_mss / pPTCP->m_cwnd);
            }
        }
    } else if (pSeg->ack == pPTCP->m_snd_una) {
        // !?! Note, tcp says don't do this... but otherwise how does a closed window become open?
        pPTCP->m_snd_wnd = (U32)(pSeg->wnd) << pPTCP->m_swnd_scale;

        // Check duplicate acks
        if (pSeg->len > 0) {
            // it's a dup ack, but with a data payload, so don't modify pPTCP->m_dup_acks
        } else if (pPTCP->m_snd_una != pPTCP->m_snd_nxt) {
            pPTCP->m_dup_acks += 1;
            if (pPTCP->m_dup_acks == 3) { // (Fast Retransmit)
#if _DEBUGMSG >= _DBG_NORMAL
                LOG_F(LS_INFO, "enter recovery");
                LOG_F(LS_INFO, "recovery retransmit");
#endif // _DEBUGMSG
                if (!transmit(pPTCP, SEGMENT_LIST_begin(pPTCP->m_slist), now)) {
                    closedown(pPTCP, ECONNABORTED);
                    return false;
                }
                pPTCP->m_recover = pPTCP->m_snd_nxt;
                U32 nInFlight = pPTCP->m_snd_nxt - pPTCP->m_snd_una;
                pPTCP->m_ssthresh = MAX(nInFlight / 2, 2 * pPTCP->m_mss);
                //LOG_F(LS_INFO) << "pPTCP->m_ssthresh: " << pPTCP->m_ssthresh << "  nInFlight: " << nInFlight << "  pPTCP->m_mss: " << pPTCP->m_mss;
                pPTCP->m_cwnd = pPTCP->m_ssthresh + 3 * pPTCP->m_mss;
            } else if (pPTCP->m_dup_acks > 3) {
                pPTCP->m_cwnd += pPTCP->m_mss;
            }
        } else {
            pPTCP->m_dup_acks = 0;
        }
    }

    // !?! A bit hacky
    if ((pPTCP->m_state == TCP_SYN_RECEIVED) && !bConnect) {
        pPTCP->m_state = TCP_ESTABLISHED;
        LOG_F(LS_INFO, "State: TCP_SYN_RECEIVED -> TCP_ESTABLISHED");
        adjustMTU(pPTCP);
        if(pPTCP->OnTcpOpen) {
            pPTCP->OnTcpOpen(pPTCP);
        }
        //notify(evOpen);
    }

    // If we make room in the send queue, notify the user
    // The goal it to make sure we always have at least enough data to fill the
    // window.  We'd like to notify the app when we are halfway to that point.
    const U32 kIdealRefillSize = (pPTCP->m_sbuf_len + pPTCP->m_rbuf_len) / 2;
    size_t snd_buffered = 0;
    //pPTCP->m_sbuf.GetBuffered(&snd_buffered);
    FIFO_GetBuffered(pPTCP->m_sbuf, &snd_buffered);
    if (pPTCP->m_bWriteEnable && (U32)(snd_buffered) < kIdealRefillSize) {
        pPTCP->m_bWriteEnable = false;
        if(pPTCP->OnTcpWriteable) {
            pPTCP->OnTcpWriteable(pPTCP);
        }
        //notify(evWrite);
    }

    // Conditions were acks must be sent:
    // 1) Segment is too old (they missed an ACK) (immediately)
    // 2) Segment is too new (we missed a segment) (immediately)
    // 3) Segment has data (so we need to ACK!) (delayed)
    // ... so the only time we don't need to ACK, is an empty segment that points to rcv_nxt!

    SendFlags sflags = sfNone;
    if (pSeg->seq != pPTCP->m_rcv_nxt) {
        sflags = sfImmediateAck; // (Fast Recovery)
    } else if (pSeg->len != 0) {
        if (pPTCP->m_ack_delay == 0) {
            sflags = sfImmediateAck;
        } else {
            sflags = sfDelayedAck;
        }
    }
#if _DEBUGMSG >= _DBG_NORMAL
    if (sflags == sfImmediateAck) {
        if (pSeg->seq > pPTCP->m_rcv_nxt) {
            LOG_F(LS_INFO, "too new");
        } else if (pSeg->seq + pSeg->len <= pPTCP->m_rcv_nxt) {
            LOG_F(LS_INFO, "too old");
        }
    }
#endif // _DEBUGMSG

    // Adjust the incoming segment to fit our receive buffer
    if (pSeg->seq < pPTCP->m_rcv_nxt) {
        U32 nAdjust = pPTCP->m_rcv_nxt - pSeg->seq;
        if (nAdjust < pSeg->len) {
            pSeg->seq += nAdjust;
            pSeg->data += nAdjust;
            pSeg->len -= nAdjust;
        } else {
            pSeg->len = 0;
        }
    }

    size_t available_space = 0;
    //pPTCP->m_rbuf.GetWriteRemaining(&available_space);
    FIFO_GetWriteRemaining(pPTCP->m_rbuf, &available_space);

    if ((pSeg->seq + pSeg->len - pPTCP->m_rcv_nxt) > (U32)(available_space)) {
        U32 nAdjust = pSeg->seq + pSeg->len - pPTCP->m_rcv_nxt - (U32)(available_space);
        if (nAdjust < pSeg->len) {
            pSeg->len -= nAdjust;
        } else {
            pSeg->len = 0;
        }
    }

    bool bIgnoreData = (pSeg->flags & FLAG_CTL) || (pPTCP->m_shutdown != SD_NONE);
    bool bNewData = false;

    if (pSeg->len > 0) {
        if (bIgnoreData) {
            if (pSeg->seq == pPTCP->m_rcv_nxt) {
                pPTCP->m_rcv_nxt += pSeg->len;
            }
        } else {
            size_t bytesWritten;
            U32 nOffset = pSeg->seq - pPTCP->m_rcv_nxt;
            if(pSeg->seq < pPTCP->m_rcv_nxt) {
                printf("(pSeg->seq, pPTCP->m_rcv_nxt) = (%u,%u)\n", pSeg->seq, pPTCP->m_rcv_nxt);
                ASSERT(pSeg->seq >= pPTCP->m_rcv_nxt);
            }
            tStreamResult result = FIFO_WriteOffset(pPTCP->m_rbuf, pSeg->data, pSeg->len, nOffset, &bytesWritten);
            ASSERT(result == SR_SUCCESS);
            ASSERT(pSeg->len == bytesWritten);
            UNUSED(result);

            if (pSeg->seq == pPTCP->m_rcv_nxt) {
                FIFO_ConsumeWriteBuffer(pPTCP->m_rbuf, pSeg->len);
                pPTCP->m_rcv_nxt += pSeg->len;
                pPTCP->m_rcv_wnd -= pSeg->len;
                bNewData = true;

                //RList::iterator it = pPTCP->m_rlist.begin();
                //while ((it != SEGMENT_LIST_end(pPTCP->m_rlist)) && (it->seq <= pPTCP->m_rcv_nxt)) {
                
                tRSSegment *it = SEGMENT_LIST_begin(pPTCP->m_rlist);
                while ((it != NULL) && (it->seq <= pPTCP->m_rcv_nxt)) {
                    if (it->seq + it->len > pPTCP->m_rcv_nxt) {
                        sflags = sfImmediateAck; // (Fast Recovery)
                        U32 nAdjust = (it->seq + it->len) - pPTCP->m_rcv_nxt;
#if _DEBUGMSG >= _DBG_NORMAL
                        memset(pTmp, 0, 1024);
                        sprintf(pTmp, "Recovered: %d  bytes (%d  ->  %d)\n", nAdjust, pPTCP->m_rcv_nxt, pPTCP->m_rcv_nxt + nAdjust);
                        LOG_F(LS_VERBOSE, pTmp);
#endif // _DEBUGMSG
                        //pPTCP->m_rbuf.ConsumeWriteBuffer(nAdjust);
                        FIFO_ConsumeWriteBuffer(pPTCP->m_rbuf, nAdjust);
                        pPTCP->m_rcv_nxt += nAdjust;
                        pPTCP->m_rcv_wnd -= nAdjust;
                    }

                    //it = pPTCP->m_rlist.erase(it);
                    tMI_DLNODE *vpNode = SEGMENT_LIST_erase(pPTCP->m_rlist, &it->DLNode);
                    free(it);
                    if(vpNode==NULL) {
                        it = NULL;
                        break;
                    }

                    it = MI_NODEENTRY(vpNode, tRSSegment, DLNode);
                }
            } else {
#if _DEBUGMSG >= _DBG_NORMAL
                memset(pTmp, 0, 1024);
                sprintf(pTmp, "Saving: %d  bytes (%d  ->  %d)\n", pSeg->len, pSeg->seq, pSeg->seq + pSeg->len);
                LOG_F(LS_VERBOSE, pTmp);
#endif // _DEBUGMSG

                //RSegment rseg;
                tRSSegment *pRSSeg = malloc(sizeof(tRSSegment));
                memset(pRSSeg, 0, sizeof(tRSSegment));
                pRSSeg->seq = pSeg->seq;
                pRSSeg->len = pSeg->len;
                //RList::iterator it = pPTCP->m_rlist.begin();
                tRSSegment *it = SEGMENT_LIST_begin(pPTCP->m_rlist);
                // while ((it != pPTCP->m_rlist.end()) && (it->seq < rseg.seq))
                while ((it != NULL) && (it->seq < pRSSeg->seq)) {
                    //++it;
                    it = MI_NODEENTRY(it->DLNode.next, tRSSegment, DLNode);
                }

                //pPTCP->m_rlist.insert(it, rseg);
                if(it!=NULL) {
                    SEGMENT_LIST_insertBefore(pPTCP->m_rlist, &it->DLNode, &pRSSeg->DLNode);
                    //SEGMENT_LIST_insertAfter(pPTCP->m_rlist, &it->DLNode, &pRSSeg->DLNode);
                }
                else {
                    //ASSERT(it!=NULL);
                    SEGMENT_LIST_push_back(pPTCP->m_rlist, &pRSSeg->DLNode);
                }
            }
        }
    }

    attemptSend(pPTCP, sflags);

    // If we have new data, notify the user
    if (bNewData && pPTCP->m_bReadEnable) {
        pPTCP->m_bReadEnable = false;
        if(pPTCP->OnTcpReadable) {
            pPTCP->OnTcpReadable(pPTCP);
        }
        //notify(evRead);
    }

    memset(pTmp, 0, 1024);
    sprintf(pTmp, "%s: process done!!\n", PTCP_WHOAMI(pPTCP->tcpId));
    LOG_F(LS_VERBOSE, pTmp);
    
    return true;
}


//bool transmit(const SList::iterator& seg, U32 now) {
bool transmit(tPseudoTcp *pPTCP, tRSSegment *seg, U32 now)
{
    ASSERT(pPTCP!=NULL);
    char pTmp[1024]= {0};
    if (seg->xmit >= ((pPTCP->m_state == TCP_ESTABLISHED) ? 15 : 30)) {
        LOG_F(LS_INFO, "too many retransmits");
        return false;
    }

    U32 nTransmit = MIN(seg->len, pPTCP->m_mss);

    while (true) {
        //LOG_F(LS_INFO, "while (true)");
        U32 seq = seg->seq;
        U8 flags = (seg->bCtrl ? FLAG_CTL : 0);
        //ASSERT(seg->seq >= pPTCP->m_snd_una);
        tWriteResult wres = PTCP_Packet( pPTCP,
                                         seq,
                                         flags,
                                         seg->seq - pPTCP->m_snd_una,
                                         nTransmit);

        if (wres == WR_SUCCESS)
            break;

        if (wres == WR_FAIL) {
            LOG_F(LS_INFO, "PTCP_Packet failed\n");
            return false;
        }

        ASSERT(wres == WR_TOO_LARGE);

        while (true) {
            if (PACKET_MAXIMUMS[pPTCP->m_msslevel + 1] == 0) {
                LOG_F(LS_INFO, "MTU too small\n");
                return false;
            }
            // !?! We need to break up all outstanding and pending PTCP_Packets and then retransmit!?!

            pPTCP->m_mss = PACKET_MAXIMUMS[++pPTCP->m_msslevel] - PACKET_OVERHEAD;
            pPTCP->m_cwnd = 2 * pPTCP->m_mss; // I added this... haven't researched actual formula
            if (pPTCP->m_mss < nTransmit) {
                nTransmit = pPTCP->m_mss;
                break;
            }
        }
#if _DEBUGMSG >= _DBG_NORMAL
        sprintf(pTmp,"Adjusting mss to %d bytes\n", pPTCP->m_mss);
        LOG_F(LS_VERBOSE, pTmp);
#endif // _DEBUGMSG
    }

    if (nTransmit < seg->len) {
        memset(pTmp, 0, 1024);
        sprintf(pTmp, "mss reduced to %d\n", pPTCP->m_mss);
        LOG_F(LS_VERBOSE, pTmp);

        //SSegment subseg(seg->seq + nTransmit, seg->len - nTransmit, seg->bCtrl);
        tRSSegment *pRSSeg = malloc(sizeof(tRSSegment));
        memset(pRSSeg, 0, sizeof(tRSSegment));
        //subseg->tstamp = seg->tstamp;
        pRSSeg->seq = seg->seq + nTransmit;
        pRSSeg->len = seg->len - nTransmit;
        pRSSeg->bCtrl = seg->bCtrl;
        pRSSeg->xmit = seg->xmit;
        seg->len = nTransmit;

        ASSERT(pRSSeg->xmit==0);
        //SList::iterator next = seg;
        //pPTCP->m_slist.insert(++next, subseg);

        tRSSegment *next = seg;
        SEGMENT_LIST_insertAfter(pPTCP->m_slist, &next->DLNode, &pRSSeg->DLNode);

    }

    if (seg->xmit == 0) {
        pPTCP->m_snd_nxt += seg->len;
    }
    seg->xmit += 1;
    //seg->tstamp = now;
    if (pPTCP->m_rto_base == 0) {
        pPTCP->m_rto_base = now;
    }

    return true;
}


void attemptSend(tPseudoTcp *pPTCP, SendFlags sflags)
{
    ASSERT(pPTCP!=NULL);
    U32 now = PTCP_Now();

    if (TimeDiff(now, pPTCP->m_lastsend) > (long)(pPTCP->m_rx_rto)) {
        pPTCP->m_cwnd = pPTCP->m_mss;
    }

#if _DEBUGMSG
    bool bFirst = true;
    UNUSED(bFirst);
#endif // _DEBUGMSG

    while (true) {
        //LOG_F(LS_INFO, "while (true)");
        U32 cwnd = pPTCP->m_cwnd;
        if ((pPTCP->m_dup_acks == 1) || (pPTCP->m_dup_acks == 2)) { // Limited Transmit
            cwnd += pPTCP->m_dup_acks * pPTCP->m_mss;
        }
        U32 nWindow = MIN(pPTCP->m_snd_wnd, cwnd);
        U32 nInFlight = pPTCP->m_snd_nxt - pPTCP->m_snd_una;
        U32 nUseable = (nInFlight < nWindow) ? (nWindow - nInFlight) : 0;

        size_t snd_buffered = 0;
        FIFO_GetBuffered(pPTCP->m_sbuf, &snd_buffered);
        U32 nAvailable =
            MIN((U32)snd_buffered - nInFlight, pPTCP->m_mss);

        if (nAvailable > nUseable) {
            if (nUseable * 4 < nWindow) {
                // RFC 813 - avoid SWS
                nAvailable = 0;
            } else {
                nAvailable = nUseable;
            }
        }

#if _DEBUGMSG >= _DBG_VERBOSE
        if (bFirst) {
            size_t available_space = 0;
            FIFO_GetWriteRemaining(pPTCP->m_sbuf, &available_space);

            bFirst = false;

            char pTmpBuf[1024]= {0};
            sprintf(pTmpBuf, "\n[cwnd: %d nWindow: %d nInFlight: %d nAvailable: %d nQueued: %zu nEmpty: %zu ssthresh: %d]\n",\
                    pPTCP->m_cwnd, nWindow, nInFlight, nAvailable, snd_buffered, available_space, pPTCP->m_ssthresh);
            LOG(LS_VERBOSE, pTmpBuf);
        }
#endif // _DEBUGMSG

        if (nAvailable == 0) {
            if (sflags == sfNone)
                return;

            // If this is an immediate ack, or the second delayed ack
            if ((sflags == sfImmediateAck) || pPTCP->m_t_ack) {
                char pTmp[1024];
                sprintf(pTmp, "%s: sfImmediateAck, attempt() -> PTCP_Packet() -> TcpWritePacket()", PTCP_WHOAMI(pPTCP->tcpId));
                LOG_F(LS_VERBOSE, pTmp);
                PTCP_Packet(pPTCP, pPTCP->m_snd_nxt, 0, 0, 0);
            } else {
                pPTCP->m_t_ack = PTCP_Now();
            }
            return;
        }

        // Nagle's algorithm.
        // If there is data already in-flight, and we haven't a full segment of
        // data ready to send then hold off until we get more to send, or the
        // in-flight data is acknowledged.
        if (pPTCP->m_use_nagling && (pPTCP->m_snd_nxt > pPTCP->m_snd_una) && (nAvailable < pPTCP->m_mss))  {
            return;
        }

        // Find the next segment to transmit
        //SList::iterator it = pPTCP->m_slist.begin();
        tRSSegment *it = SEGMENT_LIST_begin(pPTCP->m_slist);
        
#if _DEBUGMSG >= _DBG_VERBOSE
        char pTmpBuf[1024]= {0};
        sprintf(pTmpBuf, "%s pPTCP->m_slist->count = %d\n",__func__ ,pPTCP->m_slist->count);
        LOG(LS_VERBOSE, pTmpBuf);
#endif // _DEBUGMSG

        if(it) {
            SEGMENT_LIST_Dump(pPTCP->m_slist);
            while (it->xmit > 0) {
                //LOG_F(LS_INFO, "it->xmit > 0");
                //++it;
                //ASSERT(it != pPTCP->m_slist.end());
                if(it->DLNode.next != NULL) {
                    it = MI_NODEENTRY(it->DLNode.next, tRSSegment, DLNode);
                } else {
                    printf("it->DLNode=0x%08x\n", (U32)&(it->DLNode));
                    ASSERT(it->DLNode.next != NULL);
                    //printf("it->DLNode.next == NULL\n");
                    break;
                }
                ASSERT(it != NULL);
            }
            //SList::iterator seg = it;
            tRSSegment *seg = it;

            // If the segment is too large, break it into two
            if (seg->len > nAvailable) {
                //SSegment subseg(seg->seq + nAvailable, seg->len - nAvailable, seg->bCtrl);
                //printf("seg->len(%d) > nAvailable(%d)\n", seg->len, nAvailable);
                //printf("tRSSegment *pRSSeg = malloc(sizeof(tRSSegment));\n");
                tRSSegment *pRSSeg = malloc(sizeof(tRSSegment));
                memset(pRSSeg, 0, sizeof(tRSSegment));
                pRSSeg->seq = seg->seq + nAvailable;
                pRSSeg->len = seg->len - nAvailable;
                pRSSeg->bCtrl = seg->bCtrl;

                seg->len = nAvailable;

                //pPTCP->m_slist.insert(++it, subseg);
                //SEGMENT_LIST_insertBefore(pPTCP->m_slist, &it->DLNode, &pRSSeg->DLNode);
                SEGMENT_LIST_insertAfter(pPTCP->m_slist, &it->DLNode, &pRSSeg->DLNode);
                //printf("pPTCP->m_slist->count = %d\n", pPTCP->m_slist->count);
            }

            //LOG_F(LS_INFO, "transmit(pPTCP, seg, now)");
            if (!transmit(pPTCP, seg, now)) {
                LOG_F(LS_ERROR, "transmit failed");
                // TODO: consider closing socket
                return;
            }
        } else {
            LOG_F(LS_ERROR, "m_slist is empty");
            return;
        }
        sflags = sfNone;
    }
}


void closedown(tPseudoTcp *pPTCP, U32 err)
{
    LOG_F(LS_INFO, "State: TCP_CLOSED");
    ASSERT(pPTCP!=NULL);
    pPTCP->m_state = TCP_CLOSED;
    if(pPTCP->OnTcpClosed) {
        pPTCP->OnTcpClosed(pPTCP, err);
    }
    //notify(evClose, err);
}


void adjustMTU(tPseudoTcp *pPTCP)
{
    ASSERT(pPTCP!=NULL);
    // Determine our current mss level, so that we can adjust appropriately later
    for (pPTCP->m_msslevel = 0; PACKET_MAXIMUMS[pPTCP->m_msslevel + 1] > 0; ++(pPTCP->m_msslevel)) {
        if ((U16)(PACKET_MAXIMUMS[pPTCP->m_msslevel]) <= pPTCP->m_mtu_advise) {
            break;
        }
    }
    pPTCP->m_mss = pPTCP->m_mtu_advise - PACKET_OVERHEAD;
    // !?! Should we reset pPTCP->m_largest here?
#if _DEBUGMSG >= _DBG_NORMAL
    char pTmp[1024]= {0};
    sprintf(pTmp, "Adjusting mss to %d bytes\n", pPTCP->m_mss);
    LOG(LS_VERBOSE, pTmp);
#endif // _DEBUGMSG
    // Enforce minimums on ssthresh and cwnd
    pPTCP->m_ssthresh = MAX(pPTCP->m_ssthresh, 2 * pPTCP->m_mss);
    pPTCP->m_cwnd = MAX(pPTCP->m_cwnd, pPTCP->m_mss);
}


bool isReceiveBufferFull(tPseudoTcp *pPTCP)
{
    size_t available_space = 0;
    FIFO_GetWriteRemaining(pPTCP->m_rbuf, &available_space);
    return !available_space;
}


void PTCP_DisableWindowScale(tPseudoTcp *pPTCP)
{
    ASSERT(pPTCP!=NULL);
    pPTCP->m_support_wnd_scale = false;
}


void queueConnectMessage(tPseudoTcp *pPTCP)
{
    ASSERT(pPTCP!=NULL);
    //talk_base::ByteBuffer buf(talk_base::ByteBuffer::ORDER_NETWORK);
    // big-endian
    U32 i = 0;
    U8 buf[100]= {0};
    buf[i++] = CTL_CONNECT;
    if (pPTCP->m_support_wnd_scale) {
        buf[i++] = TCP_OPT_WND_SCALE;
        buf[i++] = 1;
        buf[i++] = pPTCP->m_rwnd_scale;
    }
    pPTCP->m_snd_wnd = i;
    PTCP_Queue(pPTCP, buf, i, true);
}


void parseOptions(tPseudoTcp *pPTCP, const U8* data, U32 len)
{
    ASSERT(pPTCP!=NULL);
    //std::set<uint8> options_specified;
    typedef struct tSet {
        tMI_DLNODE DLNode;
        U8 val;
    } tSet;

    tMI_DLIST options_specified={0};

    // See http://www.freesoft.org/CIE/Course/Section4/8.htm for
    // parsing the options list.
    //talk_base::ByteBuffer buf(data, len);
    U32 vLength = 0, vIndex = 0, opt_len = 0;
    U8 *buf = (U8 *)data;
    vLength = len;

    // if needed, enable below to dump memory
    /*
    {
       if(vLength==1)
          printf("Options: %02x\n", buf[0]);
       if(vLength==2)
          printf("Options: %02x %02x\n", buf[0], buf[1]);
       if(vLength==3)
          printf("Options: %02x %02x %02x\n", buf[0], buf[1], buf[2]);
    }
    */
    
    while (vLength) {
        U8 kind = TCP_OPT_EOL;
        kind = (U8) buf[vIndex++];

        if (kind == TCP_OPT_EOL) {
            // End of option list.
            break;
        } else if (kind == TCP_OPT_NOOP) {
            // No op.
            continue;
        }

        // Length of this option.
        ASSERT(len != 0);
        UNUSED(len);
        opt_len = buf[vIndex++];

        // Content of this option.
        if (opt_len <= vLength) {
            applyOption(pPTCP, kind, &buf[vIndex], opt_len);
            //buf.Consume(opt_len);
            vLength -= opt_len;
        } else {
            LOG_F(LS_INFO, "Invalid option length received.");
            return;
        }

        // options_specified.insert(kind);
        tSet  *vpSet = malloc(sizeof(tSet));
        if(!vpSet) {
            LOG_F(LS_INFO, "malloc fail.");
            return;
        }
        memset(vpSet, 0, sizeof(tSet));
        vpSet->val = kind;
        MI_DlPushHead(&options_specified, &vpSet->DLNode);
    }


    bool bFind = false;
    tMI_DLNODE *vpNode = MI_DlFirst(&options_specified);
    tSet       *vpSet;
    while(vpNode!=NULL) {
        vpSet = MI_NODEENTRY(vpNode, tSet, DLNode);
        if(vpSet) {
            if(vpSet->val == TCP_OPT_WND_SCALE) {
                bFind = true;
                break;
            }
        }
        vpNode = vpNode->next;
    }

    // The set won't be used, we should free it
    vpNode = MI_DlFirst(&options_specified);
    while(vpNode!=NULL) {
        vpSet = MI_NODEENTRY(vpNode, tSet, DLNode);
        if(vpSet) {
            vpNode = vpNode->next;
            free(vpSet);
        }
        else {
            ASSERT(vpSet!=NULL);
        }
    }
    
    //if (options_specified.find(TCP_OPT_WND_SCALE) == options_specified.end()) {
    if(bFind==false) {
        LOG_F(LS_INFO, "Peer doesn't support window scaling");

        if (pPTCP->m_rwnd_scale > 0) {
            // Peer doesn't support TCP options and window scaling.
            // Revert receive buffer size to default value.
            resizeReceiveBuffer(pPTCP, DEFAULT_RCV_BUF_SIZE);
            pPTCP->m_swnd_scale = 0;
        }
    }
}

void applyOption(tPseudoTcp *pPTCP, U8 kind, const U8* data, U32 len)
{
    ASSERT(pPTCP!=NULL);
    if (kind == TCP_OPT_MSS) {
        LOG_F(LS_INFO, "Peer specified MSS option which is not supported.");
        // TODO: Implement.
    } else if (kind == TCP_OPT_WND_SCALE) {
        // Window scale factor.
        // http://www.ietf.org/rfc/rfc1323.txt
        if (len != 1) {
            LOG_F(LS_INFO, "Invalid window scale option received.");
            return;
        }
        applyWindowScaleOption(pPTCP, data[0]);
    }
}

void applyWindowScaleOption(tPseudoTcp *pPTCP, U8 scale_factor)
{
    ASSERT(pPTCP!=NULL);
    pPTCP->m_swnd_scale = scale_factor;
}

void resizeSendBuffer(tPseudoTcp *pPTCP, U32 new_size)
{
    ASSERT(pPTCP!=NULL);
    pPTCP->m_sbuf_len = new_size;
    FIFO_SetCapacity(pPTCP->m_sbuf, new_size);
}

void resizeReceiveBuffer(tPseudoTcp *pPTCP, U32 new_size)
{
    ASSERT(pPTCP!=NULL);
    U8 scale_factor = 0;

    // Determine the scale factor such that the scaled window size can fit
    // in a 16-bit unsigned integer.
    while (new_size > 0xFFFF) {
        ++scale_factor;
        new_size >>= 1;
    }

    // Determine the proper size of the buffer.
    new_size <<= scale_factor;
    bool result = FIFO_SetCapacity(pPTCP->m_rbuf, new_size);

    // Make sure the new buffer is large enough to contain data in the old
    // buffer. This should always be true because this method is called either
    // before connection is established or when peers are exchanging connect
    // messages.
    ASSERT(result);
    UNUSED(result);
    pPTCP->m_rbuf_len = new_size;
    pPTCP->m_rwnd_scale = scale_factor;
    pPTCP->m_ssthresh = new_size;

    size_t available_space = 0;
    FIFO_GetWriteRemaining(pPTCP->m_rbuf, &available_space);
    pPTCP->m_rcv_wnd = (U32)(available_space);
}

/*****************************************************************************/

#ifdef __cplusplus
}
#endif

/**  @} */
/******************************************************************************
 *  CONFIDENTIAL
 *****************************************************************************/
