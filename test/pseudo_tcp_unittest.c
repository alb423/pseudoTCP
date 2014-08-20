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

inline S64 MillisecondTimestamp() {
   S64 ticks = 0;
   struct timeval tv;
   gettimeofday(&tv, NULL);
   ticks = 1000000LL * ((S64)(tv.tv_sec) + (S64)(tv.tv_usec));
      
   return ticks / 1000000LL;
}

void SleepMs(int msecs) {
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
      printf("current=%d, timeout=%d \n", Time(), (start + timeout)); \
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
static const int kTransferTimeoutMs = 3000;//15000;
static const int kBlockSize = 4096;
enum { MSG_LPACKET, MSG_RPACKET, MSG_LCLOCK, MSG_RCLOCK, MSG_IOCOMPLETE,
         MSG_WRITE};
         
/*** PRIVATE FUNCTION PROTOTYPES *********************************************/
// Basic Test
void OnTcpClosed_01(tPseudoTcp* pPTCP, U32 error);
void OnTcpOpen_01(tPseudoTcp* pPTCP);
void OnTcpWriteable_01(tPseudoTcp* tcp);
void OnTcpReadable_01(tPseudoTcp* tcp);
tWriteResult TcpWritePacket_01(tPseudoTcp* pPTCP, const char* buffer, size_t len);
void OnMessage(tMessage* message);

/*** PUBLIC FUNCTION DEFINITIONS *********************************************/
typedef struct tPseudoTcpForTest
{
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
} tPseudoTcpForTest;

tPseudoTcp        *_gpLocal;
tPseudoTcp        *_gpRemote;
tPseudoTcpForTest *_gpPTCPTest;

tPseudoTcpForTest * init_PseudoTcpTestBase(tIPseudoTcpNotify* notify, U32 conv)
{
   tPseudoTcpForTest *pPTCPTest = malloc(sizeof(tPseudoTcpForTest));
   memset(pPTCPTest, 0, sizeof(tPseudoTcpForTest));
   
   pPTCPTest->local_   = PTCP_Init(notify, conv);
   _gpLocal = pPTCPTest->local_;

   pPTCPTest->remote_  = PTCP_Init(notify, conv);
   _gpRemote = pPTCPTest->remote_;
   
   pPTCPTest->msq = MQueue_Init(OnMessage);

   pPTCPTest->send_stream_ = MS_Init(NULL, 1000000);
   pPTCPTest->recv_stream_ = MS_Init(NULL, 1000000);
  
   pPTCPTest->have_connected_ = false;
   pPTCPTest->have_disconnected_ = false;
   pPTCPTest->local_mtu_ = 65535;
   pPTCPTest->remote_mtu_ = 65535;
   pPTCPTest->delay_ = 0;
   pPTCPTest->loss_ = 0;
   
   _gpPTCPTest = pPTCPTest;
   return pPTCPTest;
}

void PseudoTcpTestBase_Destroy(tPseudoTcpForTest *pPTCPTest)
{
   if(pPTCPTest)
   {
      PTCP_Destroy(pPTCPTest->local_);
      PTCP_Destroy(pPTCPTest->remote_);
      MS_Destroy(pPTCPTest->send_stream_);
      MS_Destroy(pPTCPTest->recv_stream_);
      MQueue_Quit(pPTCPTest->msq);
      
      //pthread_join(pPTCPTest->msq->thread, NULL);
      //MQueue_Destroy(pPTCPTest->msq);
      free(pPTCPTest);
   }
   
   _gpLocal    = NULL;
   _gpRemote   = NULL;
   _gpPTCPTest = NULL;
}

void SetLocalMtu(int mtu) {
   PTCP_NotifyMTU(_gpLocal, mtu);
   _gpPTCPTest->local_mtu_ = mtu;
}
void SetRemoteMtu(int mtu) {
   PTCP_NotifyMTU(_gpRemote, mtu);
   _gpPTCPTest->remote_mtu_ = mtu;
}

void SetDelay(int delay) {
   _gpPTCPTest->delay_ = delay;
}

void SetLoss(int percent) {
   _gpPTCPTest->loss_ = percent;
}

void SetOptNagling(bool enable_nagles) {
   PTCP_SetOption(_gpLocal, OPT_NODELAY, !enable_nagles);
   PTCP_SetOption(_gpRemote, OPT_NODELAY, !enable_nagles);
}
void SetOptAckDelay(int ack_delay) {
   PTCP_SetOption(_gpLocal, OPT_ACKDELAY, ack_delay);
   PTCP_SetOption(_gpRemote, OPT_ACKDELAY, ack_delay);
}
void SetOptSndBuf(int size) {
   PTCP_SetOption(_gpLocal, OPT_SNDBUF, size);
   PTCP_SetOption(_gpRemote, OPT_SNDBUF, size);
}

void SetRemoteOptRcvBuf(int size) {
   PTCP_SetOption(_gpRemote, OPT_RCVBUF, size);
}
void SetLocalOptRcvBuf(int size) {
   PTCP_SetOption(_gpLocal, OPT_RCVBUF, size);
}
void DisableRemoteWindowScale() {
   PTCP_DisableWindowScale(_gpRemote);
}
void DisableLocalWindowScale() {
   PTCP_DisableWindowScale(_gpLocal);
}


void UpdateClock(tPseudoTcp* pPTCP, U32 message) {
   long interval = 0;  // NOLINT
   bool bFlag = false; 
   bFlag = PTCP_GetNextClock(pPTCP, PTCP_Now(), &interval);
   
   interval = (S32)MAX(interval, 0L);  // sometimes interval is < 0
   //talk_base::Thread::Current()->Clear(this, message);
   //talk_base::Thread::Current()->MQueue_PostDelayed(interval, this, message);
   MQueue_Clear(_gpPTCPTest->msq, message, NULL);
   MQueue_PostDelayed(_gpPTCPTest->msq, (int)interval, message, NULL);
}

void UpdateLocalClock() 
{ 
   LOG(LS_INFO, "UpdateClock(_gpLocal, MSG_LCLOCK)");
   UpdateClock(_gpLocal, MSG_LCLOCK); 
}
void UpdateRemoteClock() 
{ 
   LOG(LS_INFO, "UpdateClock(_gpRemote, MSG_RCLOCK)");
   UpdateClock(_gpRemote, MSG_RCLOCK); 
}


void OnMessage(tMessage* message) {
   LOG(LS_INFO, "OnMessage\n");
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
   free(message->pData);
   
}


// ===========================================
// Basic Test

int Connect_01() {
   int ret = PTCP_Connect(_gpLocal);
   if (ret == 0) {
      UpdateLocalClock();
   }
   return ret;
}

void Close_01() {
   PTCP_Close(_gpLocal, false);
   UpdateLocalClock();
}

void ReadData() {
   U8 block[kBlockSize];
   size_t position;
   int rcvd;
   do {
      rcvd = PTCP_Recv(_gpRemote, block, sizeof(block));
      if (rcvd != -1) {
         char pTmp[1024]={0};
         MS_Write(_gpPTCPTest->recv_stream_ , block, rcvd, NULL, NULL);
         MS_GetPosition(_gpPTCPTest->recv_stream_ , &position);
         
         sprintf(pTmp,"Received: %zu\n",position);
         LOG(LS_VERBOSE, pTmp);
      }
   } while (rcvd > 0);
}

void WriteData(bool* done) {
   size_t position, tosend;
   int sent;
   U8 block[kBlockSize];
   char pTmp[1024]={0};
   do {
      MS_GetPosition(_gpPTCPTest->send_stream_, &position);
      if (MS_Read(_gpPTCPTest->send_stream_, block, sizeof(block), &tosend, NULL) !=
      SR_EOS) {
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

void OnTcpReadable_01(tPseudoTcp* tcp) {
    if (tcp == _gpRemote) { LOG(LS_INFO, "_gpRemote"); }
    else { LOG(LS_INFO, "_gpLocal"); }
    
   // Stream bytes to the recv stream as they arrive.
   if (tcp == _gpRemote) {
      ReadData();

      // TODO: OnTcpClosed() is currently only notified on error -
      // there is no on-the-wire equivalent of TCP FIN.
      // So we fake the notification when all the data has been read.
      size_t received, required;
      MS_GetPosition(_gpPTCPTest->recv_stream_, &received);
      MS_GetSize(_gpPTCPTest->send_stream_, &required);
      if (received == required)
         OnTcpClosed_01(_gpRemote, 0);
   }
}

void OnTcpWriteable_01(tPseudoTcp* tcp) {
    if (tcp == _gpRemote) { LOG(LS_INFO, "_gpRemote"); }
    else { LOG(LS_INFO, "_gpLocal"); }
    
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

void OnTcpOpen_01(tPseudoTcp* pPTCP) {
   // Consider ourselves connected when the local side gets OnTcpOpen.
   // OnTcpWriteable isn't fired at open, so we trigger it now.
    if (pPTCP == _gpRemote) { LOG(LS_INFO, "_gpRemote"); }
    else { LOG(LS_INFO, "_gpLocal"); }
   if (pPTCP == _gpLocal) {
      _gpPTCPTest->have_connected_ = true;
      OnTcpWriteable_01(pPTCP);
   }
}

// Test derived from the base should override
//   virtual void OnTcpReadable(tPseudoTcp* tcp)
// and
//   virtual void OnTcpWritable(tPseudoTcp* tcp)
void OnTcpClosed_01(tPseudoTcp* pPTCP, U32 error) {
   // Consider ourselves closed when the remote side gets OnTcpClosed.
   // TODO: OnTcpClosed is only ever notified in case of error in
   // the current implementation.  Solicited close is not (yet) supported.
    if (pPTCP == _gpRemote) { LOG(LS_INFO, "_gpRemote"); }
    else { LOG(LS_INFO, "_gpLocal"); }
   CU_ASSERT_EQUAL(0U, error);
   if (pPTCP == _gpRemote) {
      _gpPTCPTest->have_disconnected_ = true;
   }
}


tWriteResult TcpWritePacket_01(tPseudoTcp* pPTCP, const char* buffer, size_t len) {
   
   char pTmp[1024]={0};
   if (pPTCP == _gpRemote) { sprintf(pTmp, "_gpRemote MSG_RPACKET delay_=%d", _gpPTCPTest->delay_);}
   else { sprintf(pTmp, "_gpLocal MSG_LPACKET delay_=%d", _gpPTCPTest->delay_);}
   LOG(LS_VERBOSE, pTmp);

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
      pData->pVal = malloc(len+1);
      memset(pData->pVal, 0, len+1);
      memcpy(pData->pVal, buffer, len);
      pData->len = len;
      //MQueue_PostDelayed(pPTCP->delay_, this, id, pPacket);
      MQueue_PostDelayed(_gpPTCPTest->msq, _gpPTCPTest->delay_, id, pData);
   }
   return WR_SUCCESS;
}

void TestTransfer_01(int size) {
   U32 start, elapsed;
   int i=0;
   size_t received=0;
   bool bFlag = false;
   
   // Create some dummy data to send.
   bFlag = MS_ReserveSize(_gpPTCPTest->send_stream_, size);
   CU_ASSERT_EQUAL(true, bFlag);
   for (i = 0; i < size; ++i) {
      char ch = (char)i;
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
printf("line:%d\n", __LINE__);
   // Sending will start from OnTcpWriteable and complete when all data has
   // been received.
   EXPECT_TRUE_WAIT(_gpPTCPTest->have_disconnected_, kTransferTimeoutMs);
printf("line:%d\n", __LINE__);
   
   elapsed = TimeSince(start);
   MS_GetSize(_gpPTCPTest->recv_stream_, &received);
printf("line:%d elapsed=%d, (size,received)=(%d,%zu)\n", __LINE__, elapsed, size, received);
   // Ensure we closed down OK and we got the right data.
   // TODO: Ensure the errors are cleared properly.
   //EXPECT_EQ(0, local_.GetError());
   //EXPECT_EQ(0, remote_.GetError());
   CU_ASSERT_EQUAL((size_t)size, received);
   CU_ASSERT_EQUAL(0, memcmp(MS_GetBuffer(_gpPTCPTest->send_stream_),
                     MS_GetBuffer(_gpPTCPTest->recv_stream_), size));
printf("line:%d\n", __LINE__);
   char pTmp[1024];
   if(elapsed!=0)
      sprintf(pTmp, "Transferred %zu bytes in %d ms ( %d Kbps)", received, elapsed, size * 8 / elapsed);
   else
      sprintf(pTmp, "Transferred %zu bytes in 0 ms ( 0 Kbps)", received);
      
   LOG(LS_VERBOSE, pTmp);
}


// Test the usage of MessageQueue 
void PseudoTcpTest_01_01(void)
{
/*
   U32 i, now;
   tMessageQueue *q = malloc(sizeof(tMessageQueue));
   bool bFlag = false;
   q = MQueue_Init(NULL);
   
   now = Time();
   MQueue_PostAt(q, now, NULL, 3);
   MQueue_PostAt(q, now - 2, NULL, 0);
   MQueue_PostAt(q, now - 1, NULL, 1);
   MQueue_PostAt(q, now, NULL, 4);
   MQueue_PostAt(q, now - 1, NULL, 2);

   tMessage msg;
   for (i=0; i<5; ++i) {
      memset(&msg, 0, sizeof(msg));
      bFlag = MQueue_Get(q, &msg, 0, 0);
      CU_ASSERT_EQUAL(true, bFlag);
      CU_ASSERT_EQUAL(i, msg.message_id);
   }

   CU_ASSERT_EQUAL(false, MQueue_Get(q, &msg, 0, 0));  // No more messages
   MQueue_Destroy(q);
*/   
}

void PseudoTcpTest_01_02(void)
{
   tPseudoTcpForTest *pTest;
   tIPseudoTcpNotify *notify = malloc(sizeof(tIPseudoTcpNotify));
   notify->OnTcpOpen       = (tOnTcpOpenCB)OnTcpOpen_01;
   notify->OnTcpReadable   = (tOnTcpReadableCB)OnTcpReadable_01;
   notify->OnTcpWriteable  = (tOnTcpWriteableCB)OnTcpWriteable_01;
   notify->OnTcpClosed     = (tOnTcpClosedCB)OnTcpClosed_01;
   notify->TcpWritePacket  = (tTcpWritePacketCB)TcpWritePacket_01;
   
   pTest = init_PseudoTcpTestBase(notify, 0);
   CU_ASSERT_PTR_NOT_NULL(pTest);
   
   SetLocalMtu(1500);
   SetRemoteMtu(1500);
   //TestTransfer_01(1024);
   //TestTransfer_01(1400);
    TestTransfer_01(1500);
   //TestTransfer_01(1000000);
    
   PseudoTcpTestBase_Destroy(pTest);
}



// Test for PingPong
void PseudoTcpTest_02(void) 
{
/*
   tIPseudoTcpNotify *notify = malloc(sizeof(tIPseudoTcpNotify));
   notify->OnTcpOpen       = OnTcpOpen_02;
   notify->OnTcpReadable   = OnTcpReadable_02;
   notify->OnTcpWriteable  = OnTcpWriteable_02;
   notify->OnTcpClosed     = OnTcpClosed_02;
   notify->TcpWritePacket  = TcpWritePacket_02;
*/   
}

// Test for ReceiveWindow
void PseudoTcpTest_03(void) 
{
/*
   tIPseudoTcpNotify *notify = malloc(sizeof(tIPseudoTcpNotify));
   notify->OnTcpOpen       = OnTcpOpen_03;
   notify->OnTcpReadable   = OnTcpReadable_03;
   notify->OnTcpWriteable  = OnTcpWriteable_03;
   notify->OnTcpClosed     = OnTcpClosed_03;
   notify->TcpWritePacket  = TcpWritePacket_03;
*/   
}

/*****************************************************************************/

#ifdef __cplusplus
}
#endif

/**  @} */
/******************************************************************************
 *  CONFIDENTIAL
 *****************************************************************************/
