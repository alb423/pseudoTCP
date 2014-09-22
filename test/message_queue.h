/**
 * @file    message_queue.h
 * @brief   message_queue include file
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

#ifndef __MESSAGE_QUEUE_H__
#define __MESSAGE_QUEUE_H__

#ifdef __cplusplus
extern "C" {
#endif

/*** STANDARD INCLUDES *******************************************************/
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

/*** PROJECT INCLUDES ********************************************************/
#include "mi_types.h"
#include "mi_util.h"
#include "pseudo_tcp_int.h"

/*** MACROS ******************************************************************/


/*** GLOBAL TYPES DEFINITIONS ************************************************/
enum ThreadPriority {
    PRIORITY_IDLE = -1,
    PRIORITY_NORMAL = 0,
    PRIORITY_ABOVE_NORMAL = 1,
    PRIORITY_HIGH = 2,
};

    
typedef void  (* tOnMessageCB)(void* message);

typedef struct MessageHandler {
    tOnMessageCB OnMessage;
} tMessageHandler;


typedef struct tMessageData {
    size_t   len; // U32 or U64
    U8   *pVal;
} tMessageData;


typedef struct tMessage {
    tMI_DLNODE DLNode;
    U32   ts_sensitive;
    U32   message_id;
    tMessageData *pData;
    tMessageHandler *phandler;
} tMessage;

typedef struct tDelayMessage {
    tMI_PQNODE PQNode;
    int cmsDelay;  // for debugging
    U32 msTrigger;
    U32 num;
    tMessage *pMsg;
} tDelayMessage;

typedef struct tCriticalSection {
    pthread_mutex_t mutex_;
} tCriticalSection;
    
typedef struct tEvent {
    int afd_[2];
    bool fSignaled_;
    tCriticalSection crit_;
} tEvent;
    
typedef struct tMessageQueue {
    bool fStop;
    bool fPeekKeep;
    tMessage msgPeek;
    tMI_DLIST      msgq;
    tMI_PQUEUE     dmsgq;
    U32 dmsgq_next_num;
    tOnMessageCB pOnMessageCB;
    pthread_t         thread;
    tEvent       event;
} tMessageQueue;

// Event constants 
enum DispatcherEvent {
    DE_READ    = 0x0001,
    DE_WRITE   = 0x0002,
    DE_CONNECT = 0x0004,
    DE_CLOSE   = 0x0008,
    DE_ACCEPT  = 0x0010,
};


/*** PRIVATE TYPES DEFINITIONS ***********************************************/


/*** GLOBAL VARIABLE DECLARATIONS (EXTERN) ***********************************/


/*** PUBLIC FUNCTION PROTOTYPES **********************************************/

// Message List
extern void MList_Init(tMI_DLIST *pDList);
extern tMessage *MList_Begin(tMI_DLIST *pDList);
extern tMessage *MList_End(tMI_DLIST *pDList);
extern bool MList_Empty(tMI_DLIST *pDList);
extern U32 MList_Size(tMI_DLIST *pDList);
extern tMessage *MList_Front(tMI_DLIST *pDList);
extern tMessage *MList_Back(tMI_DLIST *pDList);
extern void MList_Push_Front(tMI_DLIST *pDList, tMI_DLNODE *pNode);
extern void MList_Pop_Front(tMI_DLIST *pDList);
extern void MList_Push_Back(tMI_DLIST *pDList, tMI_DLNODE *pNode);
extern void MList_Pop_Back(tMI_DLIST *pDList);
extern tMI_DLNODE *MList_Erase(tMI_DLIST *pDList, tMI_DLNODE *pNode);


// priority Queue
extern void PQueue_Init(tMI_PQUEUE *pPQueue);
extern U32 PQueue_Size(tMI_PQUEUE *pPQueue);
extern bool PQueue_Empty(tMI_PQUEUE *pPQueue);
extern void PQueue_Push(tMI_PQUEUE *pPQueue, tMI_PQNODE *pNode);
extern tDelayMessage * PQueue_Pop(tMI_PQUEUE *pPQueue);
extern tDelayMessage * PQueue_Top(tMI_PQUEUE *pPQueue);
extern tMI_PQNODE * PQueue_Erase(tMI_PQUEUE *pQ, tMI_PQNODE *pNode);
extern void PQueue_Dump(tMI_PQUEUE *pQ); // for debug

// Message Queue
extern tMessageQueue * MQueue_Init(tOnMessageCB pCB);
extern void MQueue_Quit(tMessageQueue *pIn);
extern void MQueue_Restart(tMessageQueue *pIn);
extern void MQueue_Destroy(tMessageQueue *vpIn);


extern void MQueue_Clear(tMessageQueue *pMQueue, U32 id, tMI_DLIST* removed);
extern bool Thread_ProcessMessages(tMessageQueue *pIn, int cmsLoop);
extern void MQueue_DoDelayPost(tMessageQueue *pIn, int cmsDelay, U32 tstamp, tMessageHandler *phandler, U32 id, tMessageData *pData);

extern void MQueue_Post(tMessageQueue *pIn, tMessageHandler *phandler, U32 id, tMessageData *pData, bool time_sensitive);
extern void MQueue_PostDelayed(tMessageQueue *pIn, int cmsDelay, U32 id, tMessageData *pData);
extern void MQueue_PostAt(tMessageQueue *pIn, U32 tstamp, tMessageHandler *phandler, U32 id);

extern size_t MQueue_Size(tMessageQueue *pIn);
extern bool MQueue_Empty(tMessageQueue *pIn);
extern bool MQueue_Peek(tMessageQueue *pIn, tMessage *pmsg, int cmsWait);
extern bool MQueue_Get(tMessageQueue *pIn, tMessage *pmsg, int cmsWait, bool process_io);
extern void MQueue_ReceiveSends(tMessageQueue *pMQueue) ;

// Event
bool Event_Init(tEvent *vpEvent, bool manual_reset, bool initially_signaled);
void Event_Destroy(tEvent *vpEvent);
void Event_WakeUp(tEvent *vpEvent);
bool Event_Wait(tEvent *pEvent, int cmsWait, bool process_io);

/*****************************************************************************/

#ifdef __cplusplus
}
#endif

#endif /* End of #ifndef __SEGMENT_LIST_UNITTEST_H__ */

/******************************************************************************
 *  CONFIDENTIAL
 *****************************************************************************/
