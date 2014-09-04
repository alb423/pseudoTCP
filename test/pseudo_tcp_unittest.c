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
#include <sys/time.h>

/*** PROJECT INCLUDES ********************************************************/
#include <CUnit/CUnit.h>
#include <CUnit/Console.h>
#include "mi_types.h"
#include "memory_stream.h"
#include "message_queue.h"
#include "pseudo_tcp.h"

/*** MACROS ******************************************************************/
#define LOCAL  0
#define REMOTE 1

inline S64 MillisecondTimestamp()
{
    S64 ticks = 0;
    struct timeval tv;
    gettimeofday(&tv, NULL);
    ticks = 1000000LL * ((S64)(tv.tv_sec) + (S64)(tv.tv_usec));

    return ticks / 1000000LL;
}

void SleepMs(int msecs)
{
    struct timespec short_wait;
    struct timespec remainder;
    short_wait.tv_sec = msecs / 1000;
    short_wait.tv_nsec = (msecs % 1000) * 1000 * 1000;
    LOG(LS_INFO, "SleepMs");
    nanosleep(&short_wait, &remainder);
}

/*
 // Reference video_capture_unittest.cc
 #define WAIT_(ex, timeout, res) \
 do { \
 res = (ex); \
 S64 start = MillisecondTimestamp(); \
 printf("res=%d, start=%lld, timeout=%d Total=%lld\n", res, start, timeout, (S64)(start + timeout)); \
 while (!res && MillisecondTimestamp() < start + timeout) { \
 SleepMs(5); \
 res = (ex); \
 printf("res=%d, MillisecondTimestamp=%lld\n", res, MillisecondTimestamp()); \
 } \
 } while (0);\
 */

// Reference Guint.h
#define WAIT_(ex, timeout, res) \
do { \
    U32 start = Time(); \
    res = (ex); \
    /* printf("res=%d, start=%d, timeout=%d Total=%d\n", res, start, timeout, (start + timeout)); */\
    while (!res && (Time() < start + timeout)) { \
        /* printf("current=%d, timeout=%d \n", Time(), (start + timeout)); */ \
        MQueue_ProcessMessages(_gpPTCPTest->msq, 1); \
        res = (ex); \
    } \
} while (0);

#define EXPECT_TRUE_WAIT(ex, timeout) \
do { \
    bool res; \
    WAIT_(ex, timeout, res); \
    if (!res) { CU_ASSERT_EQUAL(true, ex); }\
} while (0);

/*** GLOBAL VARIABLE DECLARATIONS (EXTERN) ***********************************/


/*** PRIVATE TYPES DEFINITIONS ***********************************************/


/*** PRIVATE VARIABLE DECLARATIONS (STATIC) **********************************/
static const int kConnectTimeoutMs = 10000;  // ~3 * default RTO of 3000ms
static const int kTransferTimeoutMs = 15000;
static const int kBlockSize = 2048;//4096;
enum { MSG_LPACKET, MSG_RPACKET, MSG_LCLOCK, MSG_RCLOCK, MSG_IOCOMPLETE,
       MSG_WRITE
     };

/*** PRIVATE FUNCTION PROTOTYPES *********************************************/
// Basic Test
void OnTcpClosed_01(tPseudoTcp* pPTCP, U32 error);
void OnTcpOpen_01(tPseudoTcp* pPTCP);
void OnTcpWriteable_01(tPseudoTcp* tcp);
void OnTcpReadable_01(tPseudoTcp* tcp);
tWriteResult TcpWritePacket_01(tPseudoTcp* pPTCP, const char* buffer, size_t len);
void OnMessage(tMessage* message);

/*** PUBLIC FUNCTION DEFINITIONS *********************************************/
typedef struct tPseudoTcpForTestBase {
    tPseudoTcp *local_;
    tPseudoTcp *remote_;
    tMemoryStream *send_stream_;
    tMemoryStream *recv_stream_;
    tMessageQueue *msq;
    bool have_connected_;
    bool have_disconnected_;
    int local_mtu_;
    int remote_mtu_;
    int delay_;
    int loss_;
} tPseudoTcpForTestBase;

tPseudoTcp        *_gpLocal;
tPseudoTcp        *_gpRemote;
tPseudoTcpForTestBase *_gpPTCPTest;

tPseudoTcpForTestBase * PseudoTcpTestBase_Init(U32 conv)
{
    tIPseudoTcpNotify *notify = malloc(sizeof(tIPseudoTcpNotify));
    memset(notify, 0, sizeof(tIPseudoTcpNotify));
    notify->OnTcpOpen       = (tOnTcpOpenCB)OnTcpOpen_01;
    notify->OnTcpReadable   = (tOnTcpReadableCB)OnTcpReadable_01;
    notify->OnTcpWriteable  = (tOnTcpWriteableCB)OnTcpWriteable_01;
    notify->OnTcpClosed     = (tOnTcpClosedCB)OnTcpClosed_01;
    notify->TcpWritePacket  = (tTcpWritePacketCB)TcpWritePacket_01;

    tPseudoTcpForTestBase *pPTCPTest = malloc(sizeof(tPseudoTcpForTestBase));
    memset(pPTCPTest, 0, sizeof(tPseudoTcpForTestBase));

    pPTCPTest->local_   = PTCP_Init(notify, conv);
    _gpLocal = pPTCPTest->local_;

    pPTCPTest->remote_  = PTCP_Init(notify, conv);
    _gpRemote = pPTCPTest->remote_;

    //pPTCPTest->msq = MQueue_Init(OnMessage);

    pPTCPTest->send_stream_ = MS_Init(NULL, 1000000);
    pPTCPTest->recv_stream_ = MS_Init(NULL, 1000000);

    pPTCPTest->have_connected_ = false;
    pPTCPTest->have_disconnected_ = false;
    pPTCPTest->local_mtu_ = 65535;
    pPTCPTest->remote_mtu_ = 65535;
    pPTCPTest->delay_ = 0;
    pPTCPTest->loss_ = 0;

    _gpPTCPTest = pPTCPTest;
    pPTCPTest->msq = MQueue_Init(OnMessage);

    return pPTCPTest;
}

void PseudoTcpTestBase_Destroy(tPseudoTcpForTestBase *pPTCPTestBase)
{
    printf("PseudoTcpTestBase_Destroy\n");
    if(pPTCPTestBase) {
        
#if 1
        printf("Destroy  pPTCPTest->local_\n");
        if(pPTCPTestBase->local_->m_rlist) {
            printf("\tpPTCP->m_rlist->count=%d\n",pPTCPTestBase->local_->m_rlist->count);
            if(pPTCPTestBase->local_->m_rlist->count!=0) {
                SEGMENT_LIST_Dump(pPTCPTestBase->local_->m_rlist);
            }
        }
        if(pPTCPTestBase->local_->m_slist) {
            printf("\tpPTCP->m_slist->count=%d\n",pPTCPTestBase->local_->m_slist->count);
            if(pPTCPTestBase->local_->m_slist->count!=0) {
                SEGMENT_LIST_Dump(pPTCPTestBase->local_->m_slist);
            }
        }
        printf("Destroy  pPTCPTest->remote_\n");
        if(pPTCPTestBase->remote_->m_rlist) {
            printf("\tpPTCP->m_rlist->count=%d\n",pPTCPTestBase->remote_->m_rlist->count);
            if(pPTCPTestBase->remote_->m_rlist->count!=0) {
                SEGMENT_LIST_Dump(pPTCPTestBase->remote_->m_rlist);
            }
        }
        if(pPTCPTestBase->remote_->m_slist) {
            printf("\tpPTCP->m_slist->count=%d\n",pPTCPTestBase->remote_->m_slist->count);
            if(pPTCPTestBase->remote_->m_slist->count!=0) {
                SEGMENT_LIST_Dump(pPTCPTestBase->remote_->m_slist);
            }
        }

        if(pPTCPTestBase->msq){
            printf("Destroy  pPTCPTestBase->msq\n");
            printf("\tmsgq->count=%d\n",pPTCPTestBase->msq->msgq.count);
            if(pPTCPTestBase->msq->msgq.count!=0) {
                //
            }
            printf("\tdmsgq->count=%d\n",pPTCPTestBase->msq->dmsgq.count);
            if(pPTCPTestBase->msq->dmsgq.count!=0) {
                PQueue_Dump(&pPTCPTestBase->msq->dmsgq);
            }
        }
#endif
        
        MQueue_Quit(pPTCPTestBase->msq);
        
        MS_Destroy(pPTCPTestBase->send_stream_);
        MS_Destroy(pPTCPTestBase->recv_stream_);
        
        PTCP_Destroy(pPTCPTestBase->local_);
        PTCP_Destroy(pPTCPTestBase->remote_);

        MQueue_Destroy(pPTCPTestBase->msq);
        
        //pthread_t threadId =  pPTCPTest->msq->thread;
        //pthread_join(threadId, NULL);
        //pthread_join(pPTCPTest->msq->thread, NULL);
        //MQueue_Quit(pPTCPTest->msq);
        //MQueue_Destroy(pPTCPTest->msq);
        free(pPTCPTestBase);
    }
    else {
        printf("\tpPTCP is NULL\n");
    }

    _gpLocal    = NULL;
    _gpRemote   = NULL;
    _gpPTCPTest = NULL;
}

void SetLocalMtu(int mtu)
{
    PTCP_NotifyMTU(_gpLocal, mtu);
    _gpPTCPTest->local_mtu_ = mtu;
}
void SetRemoteMtu(int mtu)
{
    PTCP_NotifyMTU(_gpRemote, mtu);
    _gpPTCPTest->remote_mtu_ = mtu;
}

void SetDelay(int delay)
{
    _gpPTCPTest->delay_ = delay;
}

void SetLoss(int percent)
{
    _gpPTCPTest->loss_ = percent;
}

void SetOptNagling(bool enable_nagles)
{
    PTCP_SetOption(_gpLocal, OPT_NODELAY, !enable_nagles);
    PTCP_SetOption(_gpRemote, OPT_NODELAY, !enable_nagles);
}
void SetOptAckDelay(int ack_delay)
{
    PTCP_SetOption(_gpLocal, OPT_ACKDELAY, ack_delay);
    PTCP_SetOption(_gpRemote, OPT_ACKDELAY, ack_delay);
}
void SetOptSndBuf(int size)
{
    PTCP_SetOption(_gpLocal, OPT_SNDBUF, size);
    PTCP_SetOption(_gpRemote, OPT_SNDBUF, size);
}

void SetRemoteOptRcvBuf(int size)
{
    PTCP_SetOption(_gpRemote, OPT_RCVBUF, size);
}
void SetLocalOptRcvBuf(int size)
{
    PTCP_SetOption(_gpLocal, OPT_RCVBUF, size);
}
void DisableRemoteWindowScale()
{
    PTCP_DisableWindowScale(_gpRemote);
}
void DisableLocalWindowScale()
{
    PTCP_DisableWindowScale(_gpLocal);
}


void UpdateClock(tPseudoTcp* pPTCP, U32 message)
{
    long interval = 0;  // NOLINT
    bool bFlag = false;
    bFlag = PTCP_GetNextClock(pPTCP, PTCP_Now(), &interval);

    interval = (S32)MAX(interval, 0L);  // sometimes interval is < 0
    //talk_base::Thread::Current()->Clear(this, message);
    //talk_base::Thread::Current()->MQueue_PostDelayed(interval, this,
    if(_gpPTCPTest) {
        MQueue_Clear(_gpPTCPTest->msq, message, NULL);
        MQueue_PostDelayed(_gpPTCPTest->msq, (int)interval, message, NULL);
    }
}

void UpdateLocalClock()
{
    //LOG(LS_INFO, "UpdateClock(_gpLocal, MSG_LCLOCK)");
    UpdateClock(_gpLocal, MSG_LCLOCK);
}
void UpdateRemoteClock()
{
    //LOG(LS_INFO, "UpdateClock(_gpRemote, MSG_RCLOCK)");
    UpdateClock(_gpRemote, MSG_RCLOCK);
}


void OnMessage(tMessage* message)
{
    //LOG(LS_INFO, "OnMessage\n");
    //printf("OnMessage message->message_id=%d\n", message->message_id);
    switch (message->message_id) {
    case MSG_LPACKET: {
        PTCP_NotifyPacket(_gpLocal, message->pData->pVal, message->pData->len);
        UpdateLocalClock();
        break;
    }

    case MSG_RPACKET: {
        PTCP_NotifyPacket(_gpRemote, message->pData->pVal, message->pData->len);
        UpdateRemoteClock();
        break;
    }

    case MSG_LCLOCK:
        PTCP_NotifyClock(_gpLocal, PTCP_Now());
        UpdateLocalClock();
        break;

    case MSG_RCLOCK:
        PTCP_NotifyClock(_gpRemote, PTCP_Now());
        UpdateRemoteClock();
        break;

    default:
        break;
    }
    if(message->pData) {
        if(message->pData->pVal) {
            free(message->pData->pVal);
        }
        free(message->pData);
    }
}


// ===========================================
// Basic Test

int Connect_01()
{
    int ret = PTCP_Connect(_gpLocal);
    if (ret == 0) {
        UpdateLocalClock();
    }
    return ret;
}

void Close_01()
{
    PTCP_Close(_gpLocal, false);
    UpdateLocalClock();
}

void ReadData()
{
    U8 block[kBlockSize];
    size_t position;
    int rcvd;
    do {
        //rcvd = PTCP_Recv(_gpRemote, block, sizeof(block));
        rcvd = PTCP_Recv(_gpRemote, block, kBlockSize);
        if (rcvd != -1) {

#ifdef CHECK_COPY_BUFFER
            int i=0;
            for (i = 0; i < rcvd; ++i) {
                if(block[i]==0x00) {
                    ASSERT(block[i]!=0x00);
                }
            }
#endif
            //char pTmp[1024]={0};
            MS_Write(_gpPTCPTest->recv_stream_ , block, rcvd, NULL, NULL);
            MS_GetPosition(_gpPTCPTest->recv_stream_ , &position);

            char pTmp[256] = {0};
            sprintf(pTmp,"Received: %zu\n",position);
            LOG(LS_VERBOSE, pTmp);
        }
    } while (rcvd > 0);
}

void WriteData(bool* done)
{
    size_t position, tosend;
    int sent;
    U8 block[kBlockSize];
    char pTmp[1024]= {0};
    do {
        MS_GetPosition(_gpPTCPTest->send_stream_, &position);
        if (MS_Read(_gpPTCPTest->send_stream_, block, sizeof(block), &tosend, NULL) !=
                SR_EOS) {
#ifdef CHECK_COPY_BUFFER
            int i=0;
            for (i = 0; i < tosend; ++i) {
                if(block[i]==0x00) {
                    ASSERT(block[i]!=0x00);
                }
            }
#endif
            sent = PTCP_Send(_gpLocal, block, tosend);
            UpdateLocalClock();
            if (sent != -1) {
                MS_SetPosition(_gpPTCPTest->send_stream_, position + sent);
                memset(pTmp, 0, 1024);
                sprintf(pTmp,"Sent: %zu\n", position + sent);
                LOG(LS_VERBOSE, pTmp);
            } else {
                MS_SetPosition(_gpPTCPTest->send_stream_, position);
                LOG(LS_INFO, "Flow Controlled");
            }
        } else {
            sent = 0;
            tosend = 0;
        }
    } while (sent > 0);

    *done = (tosend == 0);
}

void OnTcpReadable_01(tPseudoTcp* tcp)
{
    //if (tcp == _gpRemote) { LOG(LS_INFO, "_gpRemote"); }
    //else { LOG(LS_INFO, "_gpLocal"); }

    // Stream bytes to the recv stream as they arrive.
    if (tcp == _gpRemote) {
        ReadData();

        // TODO: OnTcpClosed() is currently only notified on error -
        // there is no on-the-wire equivalent of TCP FIN.
        // So we fake the notification when all the data has been read.
        size_t received, required;
        MS_GetPosition(_gpPTCPTest->recv_stream_, &received);
        MS_GetSize(_gpPTCPTest->send_stream_, &required);
        if (received == required) {
            // TODO: send ack back to _gpLocal, albert.liao
            OnTcpClosed_01(_gpRemote, 0);
        }
        
    }
}

void OnTcpWriteable_01(tPseudoTcp* tcp)
{
    if (tcp == _gpRemote) {
        LOG(LS_INFO, "_gpRemote");
    } else {
        LOG(LS_INFO, "_gpLocal");
    }

    // Write bytes from the send stream when we can.
    // Shut down when we've sent everything.
    if (tcp == _gpLocal) {
        LOG(LS_INFO, "Flow Control Lifted");
        bool done;
        WriteData(&done);
        if (done) {
            Close_01();
        }
    }
}

void OnTcpOpen_01(tPseudoTcp* pPTCP)
{
    // Consider ourselves connected when the local side gets OnTcpOpen.
    // OnTcpWriteable isn't fired at open, so we trigger it now.
    if (pPTCP == _gpRemote) {
        LOG(LS_INFO, "_gpRemote");
    } else {
        LOG(LS_INFO, "_gpLocal");
    }
    if (pPTCP == _gpLocal) {
        _gpPTCPTest->have_connected_ = true;
        OnTcpWriteable_01(pPTCP);
    }
}

// Test derived from the base should override
//   virtual void OnTcpReadable(tPseudoTcp* tcp)
// and
//   virtual void OnTcpWritable(tPseudoTcp* tcp)
void OnTcpClosed_01(tPseudoTcp* pPTCP, U32 error)
{
    // Consider ourselves closed when the remote side gets OnTcpClosed.
    // TODO: OnTcpClosed is only ever notified in case of error in
    // the current implementation.  Solicited close is not (yet) supported.
    if (pPTCP == _gpRemote) { LOG(LS_INFO, "_gpRemote"); }
    else { LOG(LS_INFO, "_gpLocal"); }
    printf("%s error = %u\n", __func__, error);
    CU_ASSERT_EQUAL(0U, error);
    if (pPTCP == _gpRemote) {
        // We should send ack to client, otherwise client won't free the resource that have not acked.
        _gpPTCPTest->have_disconnected_ = true;
    }

}


tWriteResult TcpWritePacket_01(tPseudoTcp* pPTCP, const char* buffer, size_t len)
{

    char pTmp[1024]= {0};
    //   if (pPTCP == _gpRemote) { sprintf(pTmp, "_gpRemote MSG_RPACKET delay_=%d", _gpPTCPTest->delay_);}
    //   else { sprintf(pTmp, "_gpLocal MSG_LPACKET delay_=%d", _gpPTCPTest->delay_);}
    //   LOG(LS_VERBOSE, pTmp);

    // Randomly drop the desired percentage of packets.
    // Also drop packets that are larger than the configured MTU.
    if (rand() % 100 < (U32)(_gpPTCPTest->loss_)) {
        sprintf(pTmp, "Randomly dropping packet, size=%zu\n", len);
        LOG(LS_VERBOSE, pTmp);
    } else if (len > (size_t)(MIN(_gpPTCPTest->local_mtu_, _gpPTCPTest->remote_mtu_))) {
        sprintf(pTmp, "Dropping packet that exceeds path MTU, size=%zu\n", len);
        LOG(LS_VERBOSE, pTmp);
    } else {
        int id = (pPTCP == _gpLocal) ? MSG_RPACKET : MSG_LPACKET;

        tMessageData *pData = malloc(sizeof(tMessageData));
        memset(pData, 0, sizeof(tMessageData));
        pData->pVal = malloc(len+1);
        if(!pData->pVal) {
            ASSERT(pData->pVal!=NULL);
        }

        memset(pData->pVal, 0, len+1);
        memcpy(pData->pVal, buffer, len);
        pData->len = len;

#ifdef CHECK_COPY_BUFFER
        int i=0;
        int vHEADER_SIZE = 24 + 4;
        // omit the control packet
        if(pData) {
            if(pData->len > vHEADER_SIZE) {
                for (i = vHEADER_SIZE; i < pData->len-vHEADER_SIZE; ++i) {
                    if(pData->pVal[i]==0x00) {
                        ASSERT(pData->pVal[i]!=0x00);
                    }
                }
            }
        }
#endif

        //MQueue_PostDelayed(pPTCP->delay_, this, id, pPacket);
        MQueue_PostDelayed(_gpPTCPTest->msq, _gpPTCPTest->delay_, id, pData);
    }
    return WR_SUCCESS;
}

void TestTransfer_01(int size)
{
    U32 start, elapsed;
    int i=0;
    size_t received=0;
    bool bFlag = false;

    // Create some dummy data to send.
    bFlag = MS_ReserveSize(_gpPTCPTest->send_stream_, size);
    CU_ASSERT_EQUAL(true, bFlag);
    //char ch[2] = {0};

    // If the data transfered has byte which value is 0x00, we should assert
    for (i = 0; i < size; ++i) {
        char ch = (char)(i % 255+1);
        MS_Write(_gpPTCPTest->send_stream_, &ch, 1, NULL, NULL);
    }
    MS_Rewind(_gpPTCPTest->send_stream_);

    // Prepare the receive stream.
    bFlag = MS_ReserveSize(_gpPTCPTest->recv_stream_, size);
    CU_ASSERT_EQUAL(true, bFlag);

    // Connect and wait until connected.
    start = Time();
    CU_ASSERT_EQUAL(0, Connect_01());


    EXPECT_TRUE_WAIT(_gpPTCPTest->have_connected_, kConnectTimeoutMs);
    printf("line:%d _gpPTCPTest->have_connected_=%d\n", __LINE__, _gpPTCPTest->have_connected_);
    // Sending will start from OnTcpWriteable and complete when all data has
    // been received.
    EXPECT_TRUE_WAIT(_gpPTCPTest->have_disconnected_, kTransferTimeoutMs);
    printf("line:%d _gpPTCPTest->have_disconnected_=%d\n", __LINE__, _gpPTCPTest->have_disconnected_);

    elapsed = TimeSince(start);

    //sleep(1);
    MS_GetSize(_gpPTCPTest->recv_stream_, &received);
    printf("line:%d elapsed=%d, (size,received)=(%d,%zu)\n", __LINE__, elapsed, size, received);
    // Ensure we closed down OK and we got the right data.
    // TODO: Ensure the errors are cleared properly.
    //EXPECT_EQ(0, local_.GetError());
    //EXPECT_EQ(0, remote_.GetError());
    CU_ASSERT_EQUAL((size_t)size, received);
    //CU_ASSERT_EQUAL(0, memcmp(MS_GetBuffer(_gpPTCPTest->send_stream_),
    //                           MS_GetBuffer(_gpPTCPTest->recv_stream_), size));

    int vRet;
    char *pIn, *pOut;
    pIn = MS_GetBuffer(_gpPTCPTest->send_stream_);
    pOut = MS_GetBuffer(_gpPTCPTest->recv_stream_);
    vRet = memcmp(pIn, pOut, size);
    CU_ASSERT_EQUAL(0, vRet);
    if(vRet) {
        int i;
        printf("head data\n");
        for(i=0; i<10; i++) {
            printf("%02x ", (U8)pIn[i]);
        }
        printf("\n");
        for(i=0; i<10; i++) {
            printf("%02x ", (U8)pOut[i]);
        }
        printf("\n");

        printf("tail data\n");
        for(i=0; i<10; i++) {
            printf("%02x ", (U8)pIn[size-11+i]);
        }
        printf("\n");
        for(i=0; i<10; i++) {
            printf("%02x ", (U8)pOut[size-11+i]);
        }
        printf("\n");

        // Find the 1st different
        for(i=0; i<size; i++) {
            if(pIn[i]!=pOut[i]) {
                printf("item %d is different\n", i);
                break;
            }
        }

    }

    char pTmp[1024];
    if(elapsed!=0)
        sprintf(pTmp, "Transferred %zu bytes in %d ms ( %d Kbps)", received, elapsed, size * 8 / elapsed);
    else
        sprintf(pTmp, "Transferred %zu bytes in 0 ms ( 0 Kbps)", received);

    printf("%s\n",pTmp);
}



void OnMessage_01_01(tMessage* message)
{
    //LOG(LS_INFO, "OnMessage_01_01\n");

    static int i=0;
    if(i==0) {
        CU_ASSERT_EQUAL(message->message_id, 3);
    } else if(i==1) {
        CU_ASSERT_EQUAL(message->message_id, 0);
    } else if(i==2) {
        CU_ASSERT_EQUAL(message->message_id, 1);
    } else if(i==3) {
        CU_ASSERT_EQUAL(message->message_id, 4);
    } else if(i==4) {
        CU_ASSERT_EQUAL(message->message_id, 2);
    }
    //printf("message->message_id = %d\n",message->message_id);
    i++;
}



// Test the usage of MessageQueue
static void _Push_PQNode(tMI_PQUEUE *pPQueue, int id, tMessageData *pData, int cmsDelay, int msTrigger, int num)
{
    tMessage *pMsg;
    tDelayMessage *pDMsg;
    pMsg = malloc(sizeof(tMessage));
    if(pMsg) {
        memset(pMsg, 0, sizeof(tMessage));
        //tMessageData *pData = malloc(sizeof(tMessageData));
        pMsg->phandler = NULL;
        pMsg->message_id = id;
        if(pData)
            pMsg->pData = pData;

        pDMsg = malloc(sizeof(tDelayMessage));
        if(pDMsg) {
            memset(pDMsg, 0, sizeof(tDelayMessage));
            pDMsg->cmsDelay  = cmsDelay;
            pDMsg->msTrigger  = msTrigger;
            pDMsg->num  = num;
            pDMsg->pMsg  = pMsg;
        }
        PQueue_Push(pPQueue, &pDMsg->PQNode);
    }
}


void PseudoTcpTest_01_01(void)
{
    U32 i, now;
    
    tDelayMessage *pDMsg = NULL;
    tMI_PQNODE *pPQNode = NULL;
    tMI_PQNODE *pItem1, *pItem2, *pItem3, *pItem4;
    tMI_PQUEUE *pPQueue = malloc(sizeof(tMI_PQUEUE));

    memset(pPQueue, 0, sizeof(tMI_PQUEUE));
    PQueue_Init(pPQueue);

    CU_ASSERT_EQUAL(0, PQueue_Size(pPQueue));

    for (i=0; i<5; ++i) {
        _Push_PQNode(pPQueue, i, NULL, 1024, 4196, i);
    }

    CU_ASSERT_EQUAL(5, PQueue_Size(pPQueue));
    for (i=0; i<5; ++i) {
        pDMsg = PQueue_Pop(pPQueue);
        CU_ASSERT_EQUAL(i, pDMsg->pMsg->message_id);
        CU_ASSERT_EQUAL(1024, pDMsg->cmsDelay);
        CU_ASSERT_EQUAL(4196, pDMsg->msTrigger);
    }
    CU_ASSERT_EQUAL(0, PQueue_Size(pPQueue));


    for (i=0; i<10; ++i) {
        if(i%2)
            _Push_PQNode(pPQueue, i, NULL, 1024+i, 4196, i);
        else
            _Push_PQNode(pPQueue, i, NULL, 1024, 4196, i);
    }
    CU_ASSERT_EQUAL(10, PQueue_Size(pPQueue));

    pPQNode = &(PQueue_Top(pPQueue)->PQNode);
    CU_ASSERT_PTR_NOT_NULL(pPQNode);

    //    if(pPQNode->h.next)
    //        pPQNode = pPQNode->h.next;
    //    else
    pPQNode = pPQNode->v.next;

    pPQNode = PQueue_Erase(pPQueue, pPQNode);
    CU_ASSERT_PTR_NOT_NULL(pPQNode);
    CU_ASSERT_EQUAL(9, PQueue_Size(pPQueue));

    for (i=0; i<9; ++i) {
        pDMsg = PQueue_Pop(pPQueue);
        CU_ASSERT_PTR_NOT_NULL(pDMsg);
    }
    CU_ASSERT_EQUAL(0, PQueue_Size(pPQueue));


    // Test the erro found when interopate
    // Case 1, PQueue content:
    //     item1
    //     item2
    //     item3
    // remove item 1

    _Push_PQNode(pPQueue, 1, NULL, 0, 0, 1);
    _Push_PQNode(pPQueue, 2, NULL, 250, 250, 2);
    _Push_PQNode(pPQueue, 3, NULL, 100, 100, 3);

    PQueue_Dump(pPQueue);

    PQueue_Erase(pPQueue, pPQueue->head);
    CU_ASSERT_PTR_NOT_NULL(pPQueue->head->v.next);
    CU_ASSERT_PTR_NOT_NULL(pPQueue->tail->v.previous);
    PQueue_Pop(pPQueue);
    PQueue_Pop(pPQueue);
    CU_ASSERT_EQUAL(0, PQueue_Size(pPQueue));

    // Test the erro found when interopate
    // Case 1, PQueue content:
    //     item1
    //     item2
    //     item3
    // remove item 2

    _Push_PQNode(pPQueue, 1, NULL, 0, 0, 1);
    _Push_PQNode(pPQueue, 2, NULL, 250, 250, 2);
    _Push_PQNode(pPQueue, 3, NULL, 100, 100, 3);

    PQueue_Dump(pPQueue);

    PQueue_Erase(pPQueue, pPQueue->head);
    CU_ASSERT_PTR_NOT_NULL(pPQueue->head->v.next);
    CU_ASSERT_PTR_NOT_NULL(pPQueue->tail->v.previous);
    PQueue_Pop(pPQueue);
    PQueue_Pop(pPQueue);
    CU_ASSERT_EQUAL(0, PQueue_Size(pPQueue));

    // Case 3, PQueue content:
    //     item1
    //     item2
    //     item3
    // remove item 3

    _Push_PQNode(pPQueue, 1, NULL, 0, 0, 1);
    _Push_PQNode(pPQueue, 2, NULL, 250, 250, 2);
    _Push_PQNode(pPQueue, 3, NULL, 100, 100, 3);

    PQueue_Dump(pPQueue);

    PQueue_Erase(pPQueue, pPQueue->head->v.next->v.next);
    CU_ASSERT_PTR_NOT_NULL(pPQueue->head->v.next);
    CU_ASSERT_PTR_NOT_NULL(pPQueue->tail->v.previous);
    PQueue_Pop(pPQueue);
    PQueue_Pop(pPQueue);
    CU_ASSERT_EQUAL(0, PQueue_Size(pPQueue));


    // Case 4, PQueue content:
    //     item1
    //     item2  item3
    //     item4
    // remove item 1

    _Push_PQNode(pPQueue, 1, NULL, 10, 10, 1);
    _Push_PQNode(pPQueue, 2, NULL, 20, 20, 2);
    _Push_PQNode(pPQueue, 3, NULL, 20, 20, 3);
    _Push_PQNode(pPQueue, 4, NULL, 30, 30, 4);
    PQueue_Dump(pPQueue);

    pItem1 = pPQueue->head;
    pItem2 = pPQueue->head->v.next;
    pItem3 = pPQueue->head->v.next->h.next;
    pItem4 = pPQueue->head->v.next->v.next;
    PQueue_Erase(pPQueue, pPQueue->head);
    CU_ASSERT_EQUAL(pPQueue->head, pItem2);
    CU_ASSERT_EQUAL(pPQueue->head->h.next, pItem3);
    CU_ASSERT_EQUAL(pPQueue->head->v.next, pItem4);

    PQueue_Pop(pPQueue);
    PQueue_Pop(pPQueue);
    PQueue_Pop(pPQueue);
    CU_ASSERT_EQUAL(0, PQueue_Size(pPQueue));

    // Case 5, PQueue content:
    //     item1
    //     item2  item3
    //     item4
    // remove item 2

    _Push_PQNode(pPQueue, 1, NULL, 10, 10, 1);
    _Push_PQNode(pPQueue, 2, NULL, 20, 20, 2);
    _Push_PQNode(pPQueue, 3, NULL, 20, 20, 3);
    _Push_PQNode(pPQueue, 4, NULL, 30, 30, 4);
    PQueue_Dump(pPQueue);

    pItem1 = pPQueue->head;
    pItem3 = pPQueue->head->v.next->h.next;

    PQueue_Erase(pPQueue, pPQueue->head->v.next);
    CU_ASSERT_EQUAL(pPQueue->head, pItem1);
    CU_ASSERT_EQUAL(pPQueue->head->v.next, pItem3);
    PQueue_Pop(pPQueue);
    PQueue_Pop(pPQueue);
    PQueue_Pop(pPQueue);
    CU_ASSERT_EQUAL(0, PQueue_Size(pPQueue));


    // Case 6, PQueue content:
    //     item1
    //     item2  item3
    //     item4
    // remove item 2 and item 3

    _Push_PQNode(pPQueue, 1, NULL, 10, 10, 1);
    _Push_PQNode(pPQueue, 2, NULL, 20, 20, 2);
    _Push_PQNode(pPQueue, 3, NULL, 20, 20, 3);
    _Push_PQNode(pPQueue, 4, NULL, 30, 30, 4);
    PQueue_Dump(pPQueue);

    pItem1 = pPQueue->head;
    pItem2 = pPQueue->head->v.next;
    pItem3 = pPQueue->head->v.next->h.next;
    pItem4 = pPQueue->head->v.next->v.next;

    PQueue_Erase(pPQueue, pItem2);
    CU_ASSERT_EQUAL(pPQueue->head, pItem1);
    CU_ASSERT_EQUAL(pPQueue->head->v.next, pItem3);

    PQueue_Erase(pPQueue, pItem3);
    CU_ASSERT_EQUAL(pPQueue->head, pItem1);
    CU_ASSERT_EQUAL(pPQueue->head->v.next, pItem4);

    PQueue_Pop(pPQueue);
    PQueue_Pop(pPQueue);
    CU_ASSERT_EQUAL(0, PQueue_Size(pPQueue));

    // Case 7, PQueue content:
    //     item1
    //     item2
    //     item3  item4
    // remove item 1 and item 3

    _Push_PQNode(pPQueue, 1, NULL, 10, 10, 1);
    _Push_PQNode(pPQueue, 2, NULL, 20, 20, 2);
    _Push_PQNode(pPQueue, 3, NULL, 30, 30, 3);
    _Push_PQNode(pPQueue, 4, NULL, 30, 30, 4);
    PQueue_Dump(pPQueue);

    pItem1 = pPQueue->head;
    pItem2 = pPQueue->head->v.next;
    pItem3 = pPQueue->head->v.next->v.next;
    pItem4 = pPQueue->head->v.next->v.next->h.next;

    PQueue_Erase(pPQueue, pItem1);
    CU_ASSERT_EQUAL(pPQueue->head, pItem2);
    CU_ASSERT_EQUAL(pPQueue->head->v.next, pItem3);

    PQueue_Erase(pPQueue, pItem3);
    CU_ASSERT_EQUAL(pPQueue->head, pItem2);
    CU_ASSERT_EQUAL(pPQueue->head->v.next, pItem4);

    PQueue_Pop(pPQueue);
    PQueue_Pop(pPQueue);
    CU_ASSERT_EQUAL(0, PQueue_Size(pPQueue));

    // Case 8, PQueue content:
    //     item1 item2

    // remove item 1 and then insert item 3
    //     item2 item3

    // remove item 3 and item 2

    CU_ASSERT_EQUAL(0, PQueue_Size(pPQueue));
    _Push_PQNode(pPQueue, 1, NULL, 10, 10, 1);
    _Push_PQNode(pPQueue, 2, NULL, 10, 10, 2);

    pItem1 = pPQueue->head;;
    pItem2 = pPQueue->head->h.next;
    CU_ASSERT_EQUAL(pPQueue->head, pItem1);
    CU_ASSERT_EQUAL(pPQueue->tail, pItem1);

    pPQNode = PQueue_Erase(pPQueue, pItem1);
    CU_ASSERT_EQUAL(pPQNode, pItem2);
    CU_ASSERT_EQUAL(pPQueue->head, pItem2);
    CU_ASSERT_EQUAL(pPQueue->tail, pItem2);

    _Push_PQNode(pPQueue, 3, NULL, 10, 10, 3);
    pItem3 = pPQueue->head->h.next;
    pPQNode = PQueue_Erase(pPQueue, pItem3);
    CU_ASSERT_EQUAL(pPQNode, NULL);
    CU_ASSERT_EQUAL(pPQueue->head, pItem2);
    CU_ASSERT_EQUAL(pPQueue->tail, pItem2);

    pPQNode = PQueue_Erase(pPQueue, pItem2);
    CU_ASSERT_EQUAL(NULL, pPQNode);
    CU_ASSERT_EQUAL(NULL, pPQueue->head);
    CU_ASSERT_EQUAL(NULL, pPQueue->tail);


    // Test MQueue
    tMessageQueue *q = NULL;
    q = MQueue_Init(OnMessage_01_01);

    now = Time();
    MQueue_PostAt(q, now, NULL, 3);
    MQueue_PostAt(q, now - 2, NULL, 0);
    MQueue_PostAt(q, now - 1, NULL, 1);
    MQueue_PostAt(q, now, NULL, 4);
    MQueue_PostAt(q, now - 1, NULL, 2);

    // wait OnMessage_01_01() to check receive message
    sleep(1);

    MQueue_Destroy(q);

}


#pragma mark - Basic end-to-end data transfer tests

// Test the normal case of sending data from one side to the other.
void PseudoTcpTest_TestSend(void)
{
    tPseudoTcpForTestBase *pTest = PseudoTcpTestBase_Init(0);
    CU_ASSERT_PTR_NOT_NULL(pTest);

    SetLocalMtu(1500);
    SetRemoteMtu(1500);

    // When size > 1443 (1500-PACKET_OVERHEAD), the data need demutiplex
    //TestTransfer_01(1024);
    TestTransfer_01(1500);
    //TestTransfer_01(30000);
    //TestTransfer_01(1000000);

    PseudoTcpTestBase_Destroy(pTest);
}

// Test sending data with a 50 ms RTT. Transmission should take longer due
// to a slower ramp-up in send rate.
void PseudoTcpTest_TestSendWithDelay(void)
{
    tPseudoTcpForTestBase *pTest = PseudoTcpTestBase_Init(0);
    CU_ASSERT_PTR_NOT_NULL(pTest);

    SetLocalMtu(1500);
    SetRemoteMtu(1500);
    SetDelay(50);
    TestTransfer_01(1000000);
    PseudoTcpTestBase_Destroy(pTest);
}

// Test sending data with packet loss. Transmission should take much longer due
// to send back-off when loss occurs.
void PseudoTcpTest_TestSendWithLoss(void)
{
    tPseudoTcpForTestBase *pTest = PseudoTcpTestBase_Init(0);
    CU_ASSERT_PTR_NOT_NULL(pTest);

    SetLocalMtu(1500);
    SetRemoteMtu(1500);
    SetLoss(10);
    TestTransfer_01(1000000);
    //TestTransfer_01(70000);
    PseudoTcpTestBase_Destroy(pTest);
}

// Test sending data with a 50 ms RTT and 10% packet loss. Transmission should
// take much longer due to send back-off and slower detection of loss.
void PseudoTcpTest_TestSendWithDelayAndLoss(void)
{
    tPseudoTcpForTestBase *pTest = PseudoTcpTestBase_Init(0);
    CU_ASSERT_PTR_NOT_NULL(pTest);

    SetLocalMtu(1500);
    SetRemoteMtu(1500);
    SetDelay(50);
    SetLoss(10);
    TestTransfer_01(1000000);
    PseudoTcpTestBase_Destroy(pTest);
}


// Test sending data with 10% packet loss and Nagling disabled.  Transmission
// should take about the same time as with Nagling enabled.
void PseudoTcpTest_TestSendWithLossAndOptNaglingOff(void)
{
    tPseudoTcpForTestBase *pTest = PseudoTcpTestBase_Init(0);
    CU_ASSERT_PTR_NOT_NULL(pTest);

    SetLocalMtu(1500);
    SetRemoteMtu(1500);
    SetLoss(10);
    SetOptNagling(false);
    TestTransfer_01(100000);  // less data so test runs faster
    PseudoTcpTestBase_Destroy(pTest);
}

// Test sending data with 10% packet loss and Delayed ACK disabled.
// Transmission should be slightly faster than with it enabled.
void PseudoTcpTest_TestSendWithLossAndOptAckDelayOff(void)
{
    tPseudoTcpForTestBase *pTest = PseudoTcpTestBase_Init(0);
    CU_ASSERT_PTR_NOT_NULL(pTest);
    SetLocalMtu(1500);
    SetRemoteMtu(1500);
    SetLoss(10);
    SetOptAckDelay(0);
    TestTransfer_01(100000);
    PseudoTcpTestBase_Destroy(pTest);
}

// Test sending data with 50ms delay and Nagling disabled.
void PseudoTcpTest_TestSendWithDelayAndOptNaglingOff(void)
{
    tPseudoTcpForTestBase *pTest = PseudoTcpTestBase_Init(0);
    CU_ASSERT_PTR_NOT_NULL(pTest);
    SetLocalMtu(1500);
    SetRemoteMtu(1500);
    SetDelay(50);
    SetOptNagling(false);
    TestTransfer_01(100000);
}

// Test sending data with 50ms delay and Delayed ACK disabled.
void PseudoTcpTest_TestSendWithDelayAndOptAckDelayOff(void)
{
    tPseudoTcpForTestBase *pTest = PseudoTcpTestBase_Init(0);
    CU_ASSERT_PTR_NOT_NULL(pTest);
    SetLocalMtu(1500);
    SetRemoteMtu(1500);
    SetDelay(50);
    SetOptAckDelay(0);
    TestTransfer_01(100000);
}

// Test a large receive buffer with a sender that doesn't support scaling.
void PseudoTcpTest_TestSendRemoteNoWindowScale(void)
{
    tPseudoTcpForTestBase *pTest = PseudoTcpTestBase_Init(0);
    CU_ASSERT_PTR_NOT_NULL(pTest);

    SetLocalMtu(1500);
    SetRemoteMtu(1500);
    SetLocalOptRcvBuf(100000);
    DisableRemoteWindowScale();
    TestTransfer_01(1000000);
    PseudoTcpTestBase_Destroy(pTest);
}

// Test a large sender-side receive buffer with a receiver that doesn't support
// scaling.
void PseudoTcpTest_TestSendLocalNoWindowScale(void)
{
    tPseudoTcpForTestBase *pTest = PseudoTcpTestBase_Init(0);
    CU_ASSERT_PTR_NOT_NULL(pTest);

    SetLocalMtu(1500);
    SetRemoteMtu(1500);
    SetRemoteOptRcvBuf(100000);
    DisableLocalWindowScale();
    TestTransfer_01(1000000);
    PseudoTcpTestBase_Destroy(pTest);
}

// Test when both sides use window scaling.
void PseudoTcpTest_TestSendBothUseWindowScale(void)
{
    tPseudoTcpForTestBase *pTest = PseudoTcpTestBase_Init(0);
    CU_ASSERT_PTR_NOT_NULL(pTest);

    SetLocalMtu(1500);
    SetRemoteMtu(1500);
    SetRemoteOptRcvBuf(100000);
    SetLocalOptRcvBuf(100000);
    TestTransfer_01(1000000);
    PseudoTcpTestBase_Destroy(pTest);
}

// Test using a large window scale value.
void PseudoTcpTest_TestSendLargeInFlight(void)
{
    tPseudoTcpForTestBase *pTest = PseudoTcpTestBase_Init(0);
    CU_ASSERT_PTR_NOT_NULL(pTest);

    SetLocalMtu(1500);
    SetRemoteMtu(1500);
    SetRemoteOptRcvBuf(100000);
    SetLocalOptRcvBuf(100000);
    SetOptSndBuf(150000);
    TestTransfer_01(1000000);
    PseudoTcpTestBase_Destroy(pTest);
}

void PseudoTcpTest_TestSendBothUseLargeWindowScale(void)
{
    tPseudoTcpForTestBase *pTest = PseudoTcpTestBase_Init(0);
    CU_ASSERT_PTR_NOT_NULL(pTest);

    SetLocalMtu(1500);
    SetRemoteMtu(1500);
    SetRemoteOptRcvBuf(1000000);
    SetLocalOptRcvBuf(1000000);
    TestTransfer_01(10000000);
    PseudoTcpTestBase_Destroy(pTest);
}

// Test using a small receive buffer.
void PseudoTcpTest_TestSendSmallReceiveBuffer(void)
{
    tPseudoTcpForTestBase *pTest = PseudoTcpTestBase_Init(0);
    CU_ASSERT_PTR_NOT_NULL(pTest);

    SetLocalMtu(1500);
    SetRemoteMtu(1500);
    SetRemoteOptRcvBuf(10000);
    SetLocalOptRcvBuf(10000);
    TestTransfer_01(1000000);
    PseudoTcpTestBase_Destroy(pTest);
}

// Test using a very small receive buffer.
void PseudoTcpTest_TestSendVerySmallReceiveBuffer(void)
{
    tPseudoTcpForTestBase *pTest = PseudoTcpTestBase_Init(0);
    CU_ASSERT_PTR_NOT_NULL(pTest);

    SetLocalMtu(1500);
    SetRemoteMtu(1500);
    SetRemoteOptRcvBuf(100);
    SetLocalOptRcvBuf(100);
    TestTransfer_01(100000);
    PseudoTcpTestBase_Destroy(pTest);
}


#pragma mark - Ping-pong (request/response) tests

// TODO:
void SetBytesPerSend(int i)
{
    ;
}
void TestPingPong(int i, int j)
{
    ;
}
int EstimateSendWindowSize(void)
{
    return 0;
}
int EstimateReceiveWindowSize(void)
{
    return 0;
}

// Test sending <= 1x MTU of data in each ping/pong.  Should take <10ms.
void PseudoTcpTestPingPong_TestPingPong1xMtu(void)
{
    tPseudoTcpForTestBase *pTest = PseudoTcpTestBase_Init(0);
    CU_ASSERT_PTR_NOT_NULL(pTest);

    SetLocalMtu(1500);
    SetRemoteMtu(1500);
    TestPingPong(100, 100);
    PseudoTcpTestBase_Destroy(pTest);
}

// Test sending 2x-3x MTU of data in each ping/pong.  Should take <10ms.
void PseudoTcpTestPingPong_TestPingPong3xMtu(void)
{
    tPseudoTcpForTestBase *pTest = PseudoTcpTestBase_Init(0);
    CU_ASSERT_PTR_NOT_NULL(pTest);

    SetLocalMtu(1500);
    SetRemoteMtu(1500);
    TestPingPong(400, 100);
    PseudoTcpTestBase_Destroy(pTest);
}

// Test sending 1x-2x MTU of data in each ping/pong.
// Should take ~1s, due to interaction between Nagling and Delayed ACK.
void PseudoTcpTestPingPong_TestPingPong2xMtu(void)
{
    tPseudoTcpForTestBase *pTest = PseudoTcpTestBase_Init(0);
    CU_ASSERT_PTR_NOT_NULL(pTest);

    SetLocalMtu(1500);
    SetRemoteMtu(1500);
    TestPingPong(2000, 5);
    PseudoTcpTestBase_Destroy(pTest);
}

// Test sending 1x-2x MTU of data in each ping/pong with Delayed ACK off.
// Should take <10ms.
void PseudoTcpTestPingPong_TestPingPong2xMtuWithAckDelayOff(void)
{
    tPseudoTcpForTestBase *pTest = PseudoTcpTestBase_Init(0);
    CU_ASSERT_PTR_NOT_NULL(pTest);

    SetLocalMtu(1500);
    SetRemoteMtu(1500);
    SetOptAckDelay(0);
    TestPingPong(2000, 100);
    PseudoTcpTestBase_Destroy(pTest);
}

// Test sending 1x-2x MTU of data in each ping/pong with Nagling off.
// Should take <10ms.
void PseudoTcpTestPingPong_TestPingPong2xMtuWithNaglingOff(void)
{
    tPseudoTcpForTestBase *pTest = PseudoTcpTestBase_Init(0);
    CU_ASSERT_PTR_NOT_NULL(pTest);

    SetLocalMtu(1500);
    SetRemoteMtu(1500);
    SetOptNagling(false);
    TestPingPong(2000, 5);
    PseudoTcpTestBase_Destroy(pTest);
}

// Test sending a ping as pair of short (non-full) segments.
// Should take ~1s, due to Delayed ACK interaction with Nagling.
void PseudoTcpTestPingPong_TestPingPongShortSegments(void)
{
    tPseudoTcpForTestBase *pTest = PseudoTcpTestBase_Init(0);
    CU_ASSERT_PTR_NOT_NULL(pTest);

    SetLocalMtu(1500);
    SetRemoteMtu(1500);
    SetOptAckDelay(5000);
    SetBytesPerSend(50); // i.e. two Send calls per payload
    TestPingPong(100, 5);
    PseudoTcpTestBase_Destroy(pTest);
}

// Test sending ping as a pair of short (non-full) segments, with Nagling off.
// Should take <10ms.
void PseudoTcpTestPingPong_TestPingPongShortSegmentsWithNaglingOff(void)
{
    tPseudoTcpForTestBase *pTest = PseudoTcpTestBase_Init(0);
    CU_ASSERT_PTR_NOT_NULL(pTest);

    SetLocalMtu(1500);
    SetRemoteMtu(1500);
    SetOptNagling(false);
    SetBytesPerSend(50); // i.e. two Send calls per payload
    TestPingPong(100, 5);
    PseudoTcpTestBase_Destroy(pTest);
}

// Test sending <= 1x MTU of data ping/pong, in two segments, no Delayed ACK.
// Should take ~1s.
void PseudoTcpTestPingPong_TestPingPongShortSegmentsWithAckDelayOff(void)
{
    tPseudoTcpForTestBase *pTest = PseudoTcpTestBase_Init(0);
    CU_ASSERT_PTR_NOT_NULL(pTest);

    SetLocalMtu(1500);
    SetRemoteMtu(1500);
    SetBytesPerSend(50); // i.e. two Send calls per payload
    SetOptAckDelay(0);
    TestPingPong(100, 5);
    PseudoTcpTestBase_Destroy(pTest);
}


#pragma mark - Test Receive Window

// Test that receive window expands and contract correctly.
void PseudoTcpTestReceiveWindow_TestReceiveWindow(void)
{
    tPseudoTcpForTestBase *pTest = PseudoTcpTestBase_Init(0);
    CU_ASSERT_PTR_NOT_NULL(pTest);

    SetLocalMtu(1500);
    SetRemoteMtu(1500);
    SetOptNagling(false);
    SetOptAckDelay(0);
    TestTransfer_01(1024 * 1000);
    PseudoTcpTestBase_Destroy(pTest);
}

// Test setting send window size to a very small value.
void PseudoTcpTestReceiveWindow_TestSetVerySmallSendWindowSize(void)
{
    tPseudoTcpForTestBase *pTest = PseudoTcpTestBase_Init(0);
    CU_ASSERT_PTR_NOT_NULL(pTest);

    SetLocalMtu(1500);
    SetRemoteMtu(1500);
    SetOptNagling(false);
    SetOptAckDelay(0);
    SetOptSndBuf(900);
    TestTransfer_01(1024 * 1000);
    CU_ASSERT_EQUAL(900u, EstimateSendWindowSize());
    PseudoTcpTestBase_Destroy(pTest);
}

// Test setting receive window size to a value other than default.
void PseudoTcpTestReceiveWindow_TestSetReceiveWindowSize(void)
{
    tPseudoTcpForTestBase *pTest = PseudoTcpTestBase_Init(0);
    CU_ASSERT_PTR_NOT_NULL(pTest);

    SetLocalMtu(1500);
    SetRemoteMtu(1500);
    SetOptNagling(false);
    SetOptAckDelay(0);
    SetRemoteOptRcvBuf(100000);
    SetLocalOptRcvBuf(100000);
    TestTransfer_01(1024 * 1000);
    CU_ASSERT_EQUAL(100000u, EstimateReceiveWindowSize());
    PseudoTcpTestBase_Destroy(pTest);
}



/*****************************************************************************/

#ifdef __cplusplus
}
#endif

/**  @} */
/******************************************************************************
 *  CONFIDENTIAL
 *****************************************************************************/
