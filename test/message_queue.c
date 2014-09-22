/**
 * @file    message_queue.c
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
#include "mi_types.h"
#include "mi_util.h"
#include "pseudo_tcp.h"
#include "message_queue.h"
/*** MACROS ******************************************************************/


/*** GLOBAL VARIABLE DECLARATIONS (EXTERN) ***********************************/

/*** PRIVATE TYPES DEFINITIONS ***********************************************/


/*** PRIVATE VARIABLE DECLARATIONS (STATIC) **********************************/
const int kForever = -1;
const U32 kMaxMsgLatency = 150;  // 150 ms
const U32 MQID_ANY = (U32)(-1);
const U32 MQID_DISPOSE = (U32)(-2);

//pthread_t         _thread;
pthread_mutex_t   _mutex;
pthread_cond_t    _cond;

// ==== Event ====
bool event_status_;
bool is_manual_reset_;
pthread_mutex_t event_mutex_;
pthread_cond_t event_cond_;

/*** PRIVATE FUNCTION PROTOTYPES *********************************************/
void CriticalSection_Enter(tCriticalSection *pCrit);
void CriticalSection_Leave(tCriticalSection *pCrit);

/*** PUBLIC FUNCTION DEFINITIONS *********************************************/
// ==== Message Queue ====
tMessageQueue * MQueue_Init(tOnMessageCB pCB);
void MQueue_DoDelayPost(tMessageQueue *pIn, int cmsDelay, U32 tstamp, tMessageHandler *phandler, U32 id, tMessageData *pData);

void MQueue_Quit(tMessageQueue *pIn);
bool IsQuitting(tMessageQueue *pIn);

U32 MList_Size(tMI_DLIST *pDList);
U32 PQueue_Size(tMI_PQUEUE *pPQueue);
size_t MQueue_Size();
bool MQueue_Empty();


// ==== Message List ====
void MList_Init(tMI_DLIST *pDList)
{
    if(pDList!=NULL)
        MI_DlInit(pDList);
}

// Returns an iterator pointing to the first element in the list container.
tMessage *MList_Begin(tMI_DLIST *pDList)
{
    if(pDList) {
        tMI_DLNODE *vpNode = MI_DlFirst(pDList);
        if(vpNode) {
            tMessage *vpTxt;
            vpTxt = MI_NODEENTRY(vpNode, tMessage, DLNode);
            return vpTxt;
        }
    }
    return NULL;
}

// Returns an iterator referring to the past-the-end element in the list container.
tMessage *MList_End(tMI_DLIST *pDList)
{
    if(pDList) {
        tMI_DLNODE *vpNode = MI_DlLast(pDList);
        if(vpNode) {
            tMessage *vpTxt;
            vpTxt = MI_NODEENTRY(vpNode, tMessage, DLNode);
            return vpTxt;
        }
    }
    return NULL;
}

// Test whether container is empty
bool MList_Empty(tMI_DLIST *pDList)
{
    if(pDList) {
        if(MI_DlCount(pDList)==0) {
            return true;
        }
        return false;
    }
    return true;
}

// Return size
U32 MList_Size(tMI_DLIST *pDList)
{
    return MI_DlCount(pDList);
}


// Returns a reference to the first element in the list container.
tMessage *MList_Front(tMI_DLIST *pDList)
{
    if(pDList) {
        tMI_DLNODE *vpNode = MI_DlFirst(pDList);
        if(vpNode) {
            tMessage *vpTxt;
            vpTxt = MI_NODEENTRY(vpNode, tMessage, DLNode);
            return vpTxt;
        }
    }
    return NULL;
}

// Returns a reference to the last element in the list container.
tMessage *MList_Back(tMI_DLIST *pDList)
{
    if(pDList) {
        tMI_DLNODE *vpNode = MI_DlLast(pDList);
        if(vpNode) {
            tMessage *vpTxt;
            vpTxt = MI_NODEENTRY(vpNode, tMessage, DLNode);
            return vpTxt;
        }
    }
    return NULL;
}

// Inserts a new element at the beginning of the list, right before its current first element.
void MList_Push_Front(tMI_DLIST *pDList, tMI_DLNODE *pNode)
{
    MI_DlPushHead(pDList, pNode);
}

// Removes the first element in the list container.
void MList_Pop_Front(tMI_DLIST *pDList)
{
    MI_DlPopHead(pDList);
    
//    tMI_DLNODE *pNode;
//    pNode = MI_DlPopHead(pDList);
//    printf("%s node=0x%08x\n", __func__, (U32)pNode);
}

// Adds a new element at the end of the list container, after its current last element.
void MList_Push_Back(tMI_DLIST *pDList, tMI_DLNODE *pNode)
{
    MI_DlPushTail(pDList, pNode);
}

// Removes the last element in the list container
void MList_Pop_Back(tMI_DLIST *pDList)
{
    MI_DlPopTail(pDList);
}


// Delete current element
tMI_DLNODE *MList_Erase(tMI_DLIST *pDList, tMI_DLNODE *pNode)
{
//#if _DEBUGMSG >= _DBG_VERBOSE
//    char pTmpBuf[1024]= {0};
//    sprintf(pTmpBuf, "pNode=0x%08x\n", (U32)pNode);
//    LOG_F(LS_VERBOSE, pTmpBuf);
//#endif // _DEBUGMSG

    if(pDList) {
        U32 vIndex;
        tMI_DLNODE *vpNode = NULL;

        vIndex = MI_DlIndex(pDList, pNode);
        if(vIndex == MI_UTIL_ERROR)
            return NULL;

        vpNode = MI_DlNth(pDList, vIndex+1);
        if(vpNode == NULL)
            return NULL;

        MI_DlDelete(pDList, pNode);

        return vpNode;
    }

    return NULL;
}


// ==== Priority Queue ====
// http://www.cplusplus.com/reference/queue/priority_queue/?kw=priority_queue
//  its first element is always the greatest of the elements it container
// Returns a reference to the first element in the list container.

#if 1
void PQueue_Dump(tMI_PQUEUE *pQ)
{
    ;
}
#else
void PQueue_Dump(tMI_PQUEUE *pQ)
{
    tMI_PQNODE *vpTest = pQ->head;
    tMI_PQNODE *vpVNode = pQ->head;
    if(pQ->head) {
        printf("Dump PQueue (pQ->head->v.next=0x%08x) (pQ->tail->v.previous=0x%08x)\n", (U32)pQ->head->v.next, (U32)pQ->tail->v.previous);
    }
    while(vpTest) {
        printf(" 0x%08x ", (U32)vpTest);
        if(vpTest->h.next!=0) {
            vpTest = vpTest->h.next;
        } else {
            printf(" \n");
            vpTest = vpVNode->v.next;
            vpVNode = vpVNode->v.next;
        }
    }
    printf("\n");
}
#endif

// sort by cmsDelay time
static S32 _priority(const tMI_PQNODE *pNode)
{
    tDelayMessage * vpTCtxt;
    vpTCtxt = MI_NODEENTRY(pNode, tDelayMessage, PQNode);
    //return vpTCtxt->msTrigger;
    return vpTCtxt->cmsDelay;
}

void PQueue_Init(tMI_PQUEUE *pPQueue)
{
    if(pPQueue!=NULL)
        MI_PQInit(pPQueue, _priority);
}

// Return size
U32 PQueue_Size(tMI_PQUEUE *pPQueue)
{
    return MI_PQCount(pPQueue);
}

// Test whether container is empty
bool PQueue_Empty(tMI_PQUEUE *pPQueue)
{
    if(pPQueue) {
        U32 vCount = MI_PQCount(pPQueue);
        if((vCount==0) ||(vCount==MI_UTIL_ERROR)) {
            return true;
        }
    }
    return false;
}

// Removes the first element in the list container.
tDelayMessage * PQueue_Pop(tMI_PQUEUE *pQ)
{
    tMI_PQNODE *pPQNode;
    pPQNode = MI_PQPopHead(pQ);
    //printf("PQueue_Pop() head=0x%08x tail=0x%08x node:0x%08x count=%d\n", (U32)pQ->head, (U32)pQ->tail, (U32)pPQNode, (U32)pQ->count);
    PQueue_Dump(pQ);
    if(pPQNode) {
        tDelayMessage *vpTxt;
        vpTxt = MI_NODEENTRY(pPQNode, tDelayMessage, PQNode);
        return vpTxt;
    }
    return NULL;
}

// Adds a new element at the end of the list container, after its current last element.
void PQueue_Push(tMI_PQUEUE *pQ, tMI_PQNODE *pPQNode)
{
    MI_PQPushTail(pQ, pPQNode);
    //printf("PQueue_Push() head=0x%08x tail=0x%08x node:0x%08x count=%d\n", (U32)pQ->head, (U32)pQ->tail, (U32)pPQNode, (U32)pQ->count);
    PQueue_Dump(pQ);
}

tDelayMessage *PQueue_Top(tMI_PQUEUE *pQ)
{
    if(pQ) {
        tMI_PQNODE *pPQNode = pQ->head;
        //printf("PQueue_Top() head=0x%08x tail=0x%08x node:0x%08x\n", (U32)pQ->head, (U32)pQ->tail, (U32)pPQNode);
        if(pPQNode) {
            tDelayMessage *vpTxt;
            vpTxt = MI_NODEENTRY(pPQNode, tDelayMessage, PQNode);
            return vpTxt;
        }
    }
    return NULL;
}

tMI_PQNODE * PQueue_Erase(tMI_PQUEUE *pQ, tMI_PQNODE *pNode)
{
    //printf("PQueue_Erase() count=%d head=0x%08x tail=0x%08x remove=0x%08x\n", pQ->count, (U32)pQ->head, (U32)pQ->tail, (U32)pNode);
    if (pQ && pNode) {
        if (pQ->head == 0) {
            return NULL;
        } else {
            tMI_PQNODE *scan = pQ->head;
            tMI_PQNODE *pVNode = scan;
            S32 priority = pQ->priority(pNode);

            while (scan) {
                // remove node
                if(pNode == scan) {
                    tMI_PQNODE *vpN = scan;

#if 0
                    // remove head
                    if (pQ->head == vpN) {
                        if (vpN->h.next == 0) {
                            pQ->head = vpN->v.next;
                            if (vpN->v.next == 0) {
                                pQ->tail = 0;
                            } else {
                                vpN->v.next->v.previous = vpN->v.previous;
                            }
                        } else {
                            pQ->head = vpN->h.next;
                            vpN->h.next->v.previous = 0;
                            vpN->h.next->v.next = vpN->v.next;

                            if (vpN->h.previous == vpN->h.next ) {
                                vpN->h.next->h.previous = 0;
                            } else {
                                vpN->h.next->h.previous = vpN->h.previous;
                            }

                            if (vpN->v.next == 0) {
                                pQ->tail = vpN->h.next;
                            } else {
                                vpN->v.next->v.previous = vpN->h.next;
                            }
                        }
                    }
                    // remove tail
                    else if(pQ->tail == vpN) {
                        if (vpN->h.previous == 0) {
                            pQ->tail = vpN->v.previous;
                            if (vpN->v.previous == 0) {
                                pQ->head = 0;
                            } else {
                                vpN->v.previous->v.next = 0;
                                //vpN->v.previous->h.next = vpN->h.next;

                                //vpN->v.previous->v.next = vpN->v.next;
                            }
                        } else {
                            //printf("vpN->h.previous != 0\n");

                            if (vpN->v.previous != 0) {
                                vpN->v.previous->v.next = vpN->h.next;

                                if(vpN->h.next) {
                                    //     item1
                                    //     item2
                                    //     item3  item4
                                    pQ->tail = vpN->h.next;
                                    vpN->h.next->v.previous = vpN->v.previous;
                                    vpN->h.next->h.previous = vpN->h.previous;
                                } else {
                                    //     item1
                                    //     item2
                                    //     item3
                                    pQ->tail = vpN->v.previous;
                                }
                            } else {
                                // The node is head, should be handle in above case
                                ASSERT(false);
                            }
                        }
                    }
                    // remove items not head and tail
                    else {
                        if (vpN->h.next == 0) {
                            if (vpN->v.next == 0) {
                                if(vpN->h.previous) {
                                    vpN->h.previous->h.next = vpN->h.next;
                                    if(vpN->h.previous->h.previous==scan)
                                        vpN->h.previous->h.previous = 0;
                                } else {
                                    vpN->v.previous->v.next = vpN->v.next;
                                }
                            } else {
                                if (vpN->h.previous == 0) {
                                    if (vpN->v.previous == 0) {
                                        // This is head node
                                        ASSERT(pQ->head == vpN);
                                    } else {
                                        // remove item 2
                                        //     item1
                                        //     item2
                                        //     item3
                                        vpN->v.previous->v.next = vpN->v.next;
                                        vpN->v.next->v.previous = vpN->v.previous;
                                    }
                                } else {
                                    // remove item 2
                                    //     item1
                                    //     item2
                                    //     item3
                                    vpN->v.previous->v.next = vpN->v.next;
                                    vpN->v.next->v.previous = vpN->v.previous;
                                    //vpN->h.previous->h.next = vpN->h.next;
                                }
                            }
                        } else {
                            if (vpN->v.next == 0) {
                                if (vpN->h.previous == 0) {
                                    if (vpN->v.previous == 0) {
                                        // This is head node
                                        ASSERT(pQ->head == vpN);
                                    } else {
                                        vpN->v.previous->v.next = vpN->h.next;
                                    }
                                } else {
                                    vpN->h.previous->h.next = vpN->h.next;
                                    vpN->h.next->h.previous = vpN->h.previous;
                                    if (vpN->v.previous == 0) {
                                        // do nothing
                                    } else {
                                        ASSERT(vpN->v.previous != 0);
                                    }
                                }
                            } else {
                                if (vpN->h.previous == 0) {
                                    if (vpN->v.previous == 0) {
                                        // This is head node
                                        ASSERT(pQ->head == vpN);
                                    } else {
                                        vpN->h.next->h.previous = 0;
                                        vpN->v.previous->v.next = vpN->h.next;
                                        vpN->v.next->v.previous = vpN->h.next;
                                    }
                                } else {
                                    if (vpN->v.previous == 0) {
                                        ASSERT(vpN->v.previous != 0);
                                    } else {
                                        // remove item2
                                        //     item1
                                        //     item2  item3
                                        //     item4

                                        if(vpN->h.previous!=scan)
                                            vpN->h.next->h.previous = vpN->h.previous;

                                        vpN->v.previous->v.next = vpN->h.next;
                                        vpN->v.next->v.previous = vpN->h.next;

                                        vpN->h.next->v.previous = vpN->v.previous;
                                        vpN->h.next->v.next = vpN->v.next;

                                    }
                                }
                            }
                        }
                    }

#else
                    // remove item in horiental line

                    // if the item is the last item in horiental line
                    if((vpN->h.previous==0) ||
                            (vpN->h.previous==vpN)) {
                        vpN->h.previous = vpN->h.next = 0;
                        if(vpN->v.previous) {
                            vpN->v.previous->v.next = vpN->v.next;
                        }
                        if(vpN->v.next) {
                            vpN->v.next->v.previous = vpN->v.previous;
                        }

                        // change head and tail
                        if(pQ->head==vpN) {
                            pQ->head = vpN->v.next;
                        }
                        if(pQ->tail==vpN) {
                            pQ->tail = vpN->v.previous;
                        }
                    }
                    // if the item is the 1st item but not last item in horiental line
                    else if(vpN==pVNode) {
                        if(vpN->h.next) {
                            if(vpN->v.previous) {
                                vpN->v.previous->v.next = vpN->h.next;
                                vpN->h.next->v.previous = vpN->v.previous;
                            }
                            if(vpN->v.next) {
                                vpN->v.next->v.previous = vpN->h.next;
                                vpN->h.next->v.next = vpN->v.next;
                            }
                            vpN->h.next->h.previous = vpN->h.previous;
                            //vpN->h.next = vpN->h.previous = 0;

                            // change head and tail
                            if(pQ->head==vpN) {
                                pQ->head = vpN->h.next;
                            }
                            if(pQ->tail==vpN) {
                                pQ->tail = vpN->h.previous;
                            }
                        }
                    } else {
                        if(vpN->h.previous) {
                            vpN->h.previous->h.next = vpN->h.next;
                            if(vpN->h.previous->h.previous==vpN)
                                vpN->h.previous->h.previous = 0;
                        }
                        if(vpN->h.next) {
                            vpN->h.next->h.previous = vpN->h.previous;
                        }
                    }

#endif

                    pQ->count--;
                    //printf("PQueue_Erase() after erase, count=%d head=0x%08x tail=0x%08x\n", pQ->count, (U32)pQ->head, (U32)pQ->tail);

                    PQueue_Dump(pQ);

                    if (vpN->h.next == 0)
                        return vpN->v.next;
                    else
                        return vpN->h.next;

                } else {
                    S32 thisprio = pQ->priority(scan);

                    if (priority == thisprio) {
                        scan = scan->h.next;
                    } else {
                        scan = scan->v.next;
                        pVNode = scan;
                    }
                    // h.next is used to hold the element with the same priority
                }
            }
            return NULL;
        }
    }
    return NULL;
}


// ==== Message Queue ====
// A Thread in message queue
void* PreRun(void* pv)
{
    tMessageQueue *vpIn = (tMessageQueue *)pv;
    vpIn->fStop = false;
    Thread_ProcessMessages(vpIn, kForever);
    return NULL;
}

tMessageQueue * MQueue_Init(tOnMessageCB pCB)
{
    tMessageQueue *vpIn = malloc(sizeof(tMessageQueue));
    if(vpIn) {
        memset(vpIn, 0, sizeof(tMessageQueue));
        vpIn->fStop = false;
        vpIn->fPeekKeep = false;
        memset(&vpIn->msgPeek, 0, sizeof(tMessage));
        MList_Init(&vpIn->msgq);
        PQueue_Init(&vpIn->dmsgq);
        vpIn->dmsgq_next_num = 0;

        Event_Init(&vpIn->event, false, false);
        vpIn->pOnMessageCB = pCB;

        // create a thread to handle message received (pass the msg to pOnMessageCB())
        pthread_mutex_init(&_mutex, NULL);
        pthread_cond_init(&_cond, NULL);
//      pthread_attr_t attr;
//      pthread_attr_init(&attr);
//      pthread_create(&vpIn->thread, &attr, PreRun, vpIn);
//      pthread_attr_destroy(&attr);

        return vpIn;
    }
    return NULL;
}

void MQueue_Destroy(tMessageQueue *vpIn)
{
    // TODO: send signal to the MQueue thread to destroy MQueue

    if(vpIn) {
        MQueue_Quit(vpIn);
        pthread_mutex_destroy(&_mutex);
        pthread_cond_destroy(&_cond);

//#if 1
//      pthread_exit(NULL);
//#else
//      int vReturn;
//      void *res;
//      vReturn = pthread_join(&vpIn->thread, &res);
//#endif

        Event_Destroy(&vpIn->event);

        free(vpIn);
    }
}

#if 1
    void MQueue_ReceiveSends(tMessageQueue *pMQueue) { ;}
#else
void MQueue_ReceiveSends(tMessageQueue *pMQueue) {
    
    // Receive a sent message. Cleanup scenarios:
    // - thread sending exits: We don't allow this, since thread can exit
    //   only via Join, so Send must complete.
    // - thread receiving exits: Wakeup/set ready in Thread::Clear()
    // - object target cleared: Wakeup/set ready in Thread::Clear()
    
    // crit_.Enter();
    CriticalSection_Enter(&pMQueue->event.crit_);
    
    while (!MList_Empty(&pMQueue->msgq)) {
        // _SendMessage smsg = sendlist_.front();
        // sendlist_.pop_front();
        // crit_.Leave();
        // smsg.msg.phandler->OnMessage(&smsg.msg);
        // crit_.Enter();
        // *smsg.ready = true;
        // smsg.thread->socketserver()->WakeUp();
        
        tMessage vxSendMssg;
        memset(&vxSendMssg, 0, sizeof(tMessage));
        
        tMessage *pTmpMsg = MList_Front(&pMQueue->msgq);
        MList_Pop_Front(&pMQueue->msgq);
        if(pTmpMsg)
        {
            // copy the structure from pTmpMsg to pmsg
            vxSendMssg = *pTmpMsg;
            MList_Pop_Front(&pMQueue->msgq);
            // TODO: check here
            free(pTmpMsg);
        }
        
        // pthread_mutex_unlock(&_mutex);
        CriticalSection_Leave(&pMQueue->event.crit_);
        if(pMQueue->pOnMessageCB)
            pMQueue->pOnMessageCB(&vxSendMssg);
        
        //pthread_mutex_lock(&_mutex);
        CriticalSection_Enter(&pMQueue->event.crit_);
        
        //vxSendMssg.ready = true;
        //Event_WakeUp(&pMQueue->event);
    }
    
    //crit_.Leave();
    CriticalSection_Leave(&pMQueue->event.crit_);

}
#endif
    
void MQueue_Clear(tMessageQueue *pMQueue, U32 id, tMI_DLIST* removed)
{
    //CritScope cs(&crit_);
    //LOG_F(LS_INFO, "pthread_mutex_lock _mutex");
    //pthread_mutex_lock(&_mutex);
    
    CriticalSection_Enter(&pMQueue->event.crit_);
    //LOG_F(LS_INFO, "MQueue_Clear");
    // Remove messages with phandler
    if (!pMQueue)
        return ;

    if (pMQueue->fPeekKeep &&
            pMQueue->msgPeek.message_id == id) {
        //pMQueue->msgPeek.Match(phandler, id)) {

        if (removed) {
            MList_Push_Back(removed, &pMQueue->msgPeek.DLNode);
        } else {
            // delete msgPeek.pdata;
            if(pMQueue->msgPeek.pData) {
                free(pMQueue->msgPeek.pData->pVal);
                free(pMQueue->msgPeek.pData);
                pMQueue->msgPeek.pData = NULL;
            }
        }
        pMQueue->fPeekKeep = false;
    }

    // Remove from ordered message queue

    tMessage *pMsg = MList_Begin(&pMQueue->msgq);
    if(pMsg!=NULL) {
        tMI_DLNODE *pDLNode = &pMsg->DLNode;
        for (; pDLNode != NULL;) {
            //printf("pMQueue:: pMsg->message_id=%d, id=%d\n", pMsg->message_id, id);
            pMsg = MI_NODEENTRY(pDLNode, tMessage, DLNode);
            //if (it->Match(phandler, id)) {
            if (pMsg->message_id == id) {
                if (removed) {
                    //removed->push_back(*it);
                    MList_Push_Back(removed, &pMsg->DLNode);
                } else {
                    //delete it->pdata;
                    if(pMsg->pData) {
                        if(pMsg->pData->pVal) {
                            free(pMsg->pData->pVal);
                        }
                        free(pMsg->pData);
                        pMsg->pData = NULL;
                    }
                }
                
                //it = msgq_.erase(it);
                pDLNode = MList_Erase(&pMQueue->msgq, pDLNode);
                // TODO: check here
                //if(!removed) free(pMsg);
                
            } else {
                // ++it;
                pDLNode = MI_DlNext(pDLNode);
            }
        }
    }

    // Remove from priority queue. Not directly iterable, so use this approach

    //PriorityQueue::container_type::iterator new_end = dmsgq_.container().begin();
    //for (PriorityQueue::container_type::iterator it = new_end;
    //it != dmsgq_.container().end(); ++it) {

    tDelayMessage *pDelayMsg = PQueue_Top(&pMQueue->dmsgq);
    if(pDelayMsg!=NULL) {
        tMI_PQNODE *pPQNode = &pDelayMsg->PQNode;

        if(pPQNode) {
            for(; pPQNode != NULL ; ) {
                //LOG_F(LS_INFO, "pPQNode != NULL");
                do {
                    pDelayMsg = MI_NODEENTRY(pPQNode, tDelayMessage, PQNode);
                    if(pDelayMsg) {
                        pMsg = pDelayMsg->pMsg;
                        //if (it->msg_.Match(phandler, id)) {
                        //printf("PQueue:: pMsg->message_id=%d, id=%d\n", pMsg->message_id, id);
                        if (pMsg) {
                            if (pMsg->message_id == id) {
                                if (removed) {
                                    //removed->push_back(it->msg_);
                                    MList_Push_Back(removed, &pMsg->DLNode);
                                } else {
                                    //delete it->msg_.pdata;
                                    if(pMsg->pData) {
                                        if(pMsg->pData->pVal) {
                                            free(pMsg->pData->pVal);
                                        }
                                        free(pMsg->pData);
                                    }
                                    free(pMsg);
                                }

                                // remove item in queue
                                pPQNode = PQueue_Erase(&pMQueue->dmsgq, pPQNode);
                                free(pDelayMsg);
                                break;
                            }
                        }
                    }
                    //*new_end++ = *it;
                    if (pPQNode->h.next == 0) {
                        if(pPQNode == pPQNode->v.next) {
                            pPQNode = NULL;
                        }
                        else {
                            pPQNode = pPQNode->v.next;
                        }
                    } else {
                        if(pPQNode == pPQNode->h.next) {
                            pPQNode = NULL;
                        }
                        else {
                            pPQNode = pPQNode->h.next;
                        }
                    }
                    //printf("PQueue:: pPQNode=0x%08x\n", (U32)pPQNode);

                } while (0);
            }
        }
    }

    //dmsgq_.container().erase(new_end, dmsgq_.container().end());
    //dmsgq_.reheap();
    //LOG_F(LS_INFO, "pthread_mutex_unlock _mutex");
    //pthread_mutex_unlock(&_mutex);
    CriticalSection_Leave(&pMQueue->event.crit_);
}

void MQueue_Dispatch(tMessage *pmsg) {
    if(pmsg->phandler) {
        // 此處的 OnMessage 並非 unittest 內的 OnMessage
        pmsg->phandler->OnMessage(pmsg);
    }
}
    
void MQueue_Post(tMessageQueue *pIn, tMessageHandler *phandler, U32 id, tMessageData *pData, bool time_sensitive)
{
    if (pIn->fStop)
        return;

    // Keep thread safe
    // Add to the priority queue. Gets sorted soonest first.
    // Signal for the multiplexer to return.

    //CritScope cs(&crit_);
    CriticalSection_Enter(&pIn->event.crit_);
    
    //LOG_F(LS_INFO, "pthread_mutex_lock _mutex");
    //pthread_mutex_lock(&_mutex);
    tMessage *pMsg = malloc(sizeof(tMessage));
    memset(pMsg, 0, sizeof(tMessage));
    pMsg->phandler = phandler;
    pMsg->message_id = id;
    pMsg->pData = pData;
    if (time_sensitive) {
        pMsg->ts_sensitive = Time() + kMaxMsgLatency;
    }
    //msgq_.push_back(msg);
    MList_Push_Back(&pIn->msgq, &pMsg->DLNode);

    //ss_->WakeUp();
    Event_WakeUp(&pIn->event);
    //LOG_F(LS_INFO, "pthread_mutex_unlock _mutex");
    //pthread_mutex_unlock(&_mutex);
    
    CriticalSection_Leave(&pIn->event.crit_);
}


void MQueue_DoDelayPost(tMessageQueue *pIn, int cmsDelay, U32 tstamp,
                        tMessageHandler *phandler, U32 id, tMessageData *pData)
{
    ASSERT(pIn != NULL);

//    pthread_mutex_lock(&_mutex);
//    if (pIn->fStop) {
//        pthread_mutex_unlock(&_mutex);
//        return;
//    }

    CriticalSection_Enter(&pIn->event.crit_);
    if(pIn->fStop) {
        CriticalSection_Leave(&pIn->event.crit_);
        return;
    }
    
#ifdef CHECK_COPY_BUFFER
    int i=0;
    int vHEADER_SIZE = 24 + 4;
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
    // Keep thread safe
    // Add to the priority queue. Gets sorted soonest first.
    // Signal for the multiplexer to return.

    //CritScope cs(&crit_);
    //LOG_F(LS_INFO, "pthread_mutex_lock _mutex");
    //pthread_mutex_lock(&_mutex);
    tMessage *pMsg = malloc(sizeof(tMessage));
    if(pMsg) {
        memset(pMsg, 0, sizeof(tMessage));
        pMsg->phandler = phandler;
        pMsg->message_id = id;
        pMsg->pData = pData;

        tDelayMessage  *pDMsg = malloc(sizeof(tDelayMessage));
        if(pDMsg) {
            memset(pDMsg, 0, sizeof(tDelayMessage));
            pDMsg->cmsDelay  = cmsDelay;
            pDMsg->msTrigger  = tstamp;
            pDMsg->num  = pIn->dmsgq_next_num;
            pDMsg->pMsg  = pMsg;

            //dmsgq_.push(dmsg);
            //LOG_F(LS_INFO, "PQueue_Push");
            //printf("&pIn->dmsgq=0x%x pDMsg=0x%x pDMsg->PQNode=0x%x\n", (U32)&pIn->dmsgq, (U32)pDMsg, (U32)&pDMsg->PQNode);
            PQueue_Push(&pIn->dmsgq, &pDMsg->PQNode);
        } else {
            free(pMsg);
            ASSERT(pDMsg!=NULL);
        }
    }

    // If this message queue processes 1 message every millisecond for 50 days,
    // we will wrap this number.  Even then, only messages with identical times
    // will be misordered, and then only briefly.  This is probably ok.
    //VERIFY(0 != ++dmsgq_next_num_);
    //ss_->WakeUp();
    VERIFY(0 != ++pIn->dmsgq_next_num);
    Event_WakeUp(&pIn->event);
    //LOG_F(LS_INFO, "pthread_mutex_unlock _mutex");
    //pthread_mutex_unlock(&_mutex);
    CriticalSection_Leave(&pIn->event.crit_);
}

void MQueue_PostDelayed(tMessageQueue *pIn, int cmsDelay, U32 id, tMessageData *pData)
{
    return MQueue_DoDelayPost(pIn, cmsDelay, TimeAfter(cmsDelay), NULL, id, pData);
}

void MQueue_PostAt(tMessageQueue *pIn, U32 tstamp, tMessageHandler *phandler, U32 id)
{
    return MQueue_DoDelayPost(pIn, TimeUntil(tstamp), tstamp, phandler, id, NULL);
}

void MQueue_Quit(tMessageQueue *pIn)
{
    LOG_F(LS_VERBOSE, "MQueue_Quit");
    pIn->fStop = true;
    //ss_->WakeUp();
    Event_WakeUp(&pIn->event);
}

void MQueue_Restart(tMessageQueue *pIn)
{
    LOG_F(LS_VERBOSE, "MQueue_Restart");
    pIn->fStop = false;
}


bool IsQuitting(tMessageQueue *pIn)
{
    return pIn->fStop;
}

size_t MQueue_Size(tMessageQueue *pIn)
{
    //CritScope cs(&crit_);  // msgq_.size() is not thread safe.
    

//    LOG_F(LS_INFO, "pthread_mutex_lock _mutex");
//    pthread_mutex_lock(&_mutex);
    CriticalSection_Enter(&pIn->event.crit_);
    int i;
    i = MList_Size(&pIn->msgq) + PQueue_Size(&pIn->dmsgq) + (pIn->fPeekKeep ? 1u : 0u);
//    LOG_F(LS_INFO, "pthread_mutex_unlock _mutex");
//    pthread_mutex_unlock(&_mutex);
    CriticalSection_Leave(&pIn->event.crit_);
    return i;
}

bool MQueue_Empty(tMessageQueue *pIn)
{
    return MQueue_Size(pIn) == 0u;
}

bool MQueue_Peek(tMessageQueue *pIn, tMessage *pmsg, int cmsWait)
{
    if (pIn->fPeekKeep) {
        *pmsg = pIn->msgPeek;
        return true;
    }
    if (!MQueue_Get(pIn, pmsg, cmsWait, true))
        return false;
    pIn->msgPeek = *pmsg;
    pIn->fPeekKeep = true;
    return true;
}

bool MQueue_Get(tMessageQueue *pIn, tMessage *pmsg, int cmsWait, bool process_io)
{
    // Return and clear peek if present
    // Always return the peek if it exists so there is Peek/Get symmetry
    
    if (pIn->fPeekKeep) {
        *pmsg = pIn->msgPeek;
        pIn->fPeekKeep = false;
        return true;
    }

    // Get w/wait + timer scan / dispatch + socket / event multiplexer dispatch

    int cmsTotal = cmsWait;
    int cmsElapsed = 0;
    U32 msStart = Time();
    U32 msCurrent = msStart;
    
    while (true) {
        // Check for sent messages
        MQueue_ReceiveSends(pIn);

        // Check for posted events
        int cmsDelayNext = kForever;
        bool first_pass = true;
        while (true) {
            // All queue operations need to be locked, but nothing else in this loop
            // (specifically handling disposed message) can happen inside the crit.
            // Otherwise, disposed MessageHandlers will cause deadlocks.

            //pthread_mutex_lock(&_mutex);
            CriticalSection_Enter(&pIn->event.crit_);
            {
                //CritScope cs(&crit_);
                //LOG_F(LS_INFO, "pthread_mutex_lock _mutex");
                //pthread_mutex_lock(&_mutex);

                // On the first pass, check for delayed messages that have been
                // triggered and calculate the next trigger time.
                if (first_pass) {
                    first_pass = false;
                    while (!PQueue_Empty(&pIn->dmsgq)) {
                        tDelayMessage *pDelayMsg = PQueue_Top(&pIn->dmsgq);
                        if(pDelayMsg) {

                            if (TimeIsLater(msCurrent, pDelayMsg->msTrigger)) {
                                cmsDelayNext = TimeDiff(pDelayMsg->msTrigger, msCurrent);
                                break;
                            }
                            //printf("__LINE__ : %d 0x%x\n", __LINE__, (U32)&pIn->dmsgq);
                            MList_Push_Back(&pIn->msgq, &(pDelayMsg->pMsg->DLNode));
                            PQueue_Pop(&pIn->dmsgq);
                            free(pDelayMsg);
                        }
                        else
                        {
                            ASSERT(pDelayMsg!=NULL);
                        }
                    }
                }
                // Pull a message off the message queue, if available.
                if (MList_Empty(&pIn->msgq)) {
                    //LOG_F(LS_INFO, "pthread_mutex_unlock _mutex");
                    pthread_mutex_unlock(&_mutex);
                    break;
                } else {
                    tMessage *pTmpMsg = MList_Front(&pIn->msgq);
                    if(pTmpMsg)
                    {
                        // copy the structure from pTmpMsg to pmsg
                        *pmsg = *pTmpMsg;
                        MList_Pop_Front(&pIn->msgq);
                        // TODO: check here
                        free(pTmpMsg);
                    }
                }

            }  // crit_ is released here.
            //LOG_F(LS_INFO, "pthread_mutex_unlock _mutex");
            //pthread_mutex_unlock(&_mutex);
            CriticalSection_Leave(&pIn->event.crit_);

            // Log a warning for time-sensitive messages that we're late to deliver.
            if (pmsg->ts_sensitive) {
                S32 delay = TimeDiff(msCurrent, pmsg->ts_sensitive);
                if (delay > 0) {
                    char pTmp[1024] = {0};
                    sprintf(pTmp, "id: %d  delay: %d  ms\n", pmsg->message_id, (delay + kMaxMsgLatency));
                    LOG_F(LS_VERBOSE, pTmp);
                    //LOG_F(LS_WARNING) << "id: " << pmsg->message_id << "  delay: "
                    //                << (delay + kMaxMsgLatency) << "ms";
                }
            }

            // If this was a dispose message, delete it and skip it.
            if (MQID_DISPOSE == pmsg->message_id) {
                ASSERT(NULL == pmsg->phandler);
                //delete pmsg->pdata;
                if(pmsg->pData) {
                    if(pmsg->pData->pVal) {
                        free(pmsg->pData->pVal);
                    }
                    free(pmsg->pData);
                    pmsg->pData = NULL;
                }
                //*pmsg = Message();
                //memset(pmsg, 0, sizeof(*pmsg));
                memset(pmsg, 0, sizeof(tMessage));
                continue;
            }
            // Now we return a pmsg with correct data
            return true;
        }

        if (pIn->fStop)
            break;

        // Which is shorter, the delay wait or the asked wait?

        int cmsNext;
        if (cmsWait == kForever) {
            cmsNext = cmsDelayNext;
        } else {
            cmsNext = MAX(0, cmsTotal - cmsElapsed);
            if ((cmsDelayNext != kForever) && (cmsDelayNext < cmsNext))
                cmsNext = cmsDelayNext;
        }

        // Wait and multiplex in the meantime
        //if (!ss_->Wait(cmsNext, process_io))
        //   return false;
        if(!Event_Wait(&pIn->event, cmsNext, process_io))
            return false;

        // If the specified timeout expired, return

        msCurrent = Time();
        cmsElapsed = TimeDiff(msCurrent, msStart);
        if (cmsWait != kForever) {
            if (cmsElapsed >= cmsWait)
                return false;
        }
    }
    return false;
}



// ==== Event ====
// reference libjingle_tests\....\physicalsocketserver.cc
#include "unistd.h"
#include <sys/types.h>          /* See NOTES */
#include <sys/socket.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/time.h>
#include <sys/select.h>
#include <unistd.h>
#include <signal.h>
bool fWait_;

void CriticalSection_Init(tCriticalSection *pCrit) {
    if(!pCrit)
        return;
    
    pthread_mutexattr_t mutex_attribute;
    pthread_mutexattr_init(&mutex_attribute);
    pthread_mutexattr_settype(&mutex_attribute, PTHREAD_MUTEX_RECURSIVE);
    pthread_mutex_init(&pCrit->mutex_, &mutex_attribute);
    pthread_mutexattr_destroy(&mutex_attribute);
}

void CriticalSection_Destroy(tCriticalSection *pCrit) {
    if(!pCrit)
        return;
    
    pthread_mutex_destroy(&pCrit->mutex_);
}
    
void CriticalSection_Enter(tCriticalSection *pCrit) {
    if(!pCrit)
        return;
    
    pthread_mutex_lock(&pCrit->mutex_);
    //TRACK_OWNER(thread_ = pthread_self());
}
    
bool CriticalSection_TryEnter(tCriticalSection *pCrit) {
    if(!pCrit)
        return false;
    
    if (pthread_mutex_trylock(&pCrit->mutex_) == 0) {
        //TRACK_OWNER(thread_ = pthread_self());
        return true;
    }
    return false;
}
    
void CriticalSection_Leave(tCriticalSection *pCrit) {
    if(!pCrit)
        return;
    
    //TRACK_OWNER(thread_ = 0);
    pthread_mutex_unlock(&pCrit->mutex_);
}
    
bool Event_Init(tEvent *pEvent, bool manual_reset, bool initially_signaled)
{
    if(!pEvent) {
        return false;
    }
    
    pEvent->fSignaled_ = false;
    if (pipe(pEvent->afd_) < 0) {
        LOG_F(LS_ERROR, "pipe failed");
    }
    
    CriticalSection_Init(&pEvent->crit_);
    return true;
}

void Event_Destroy(tEvent *pEvent)
{
    if(pEvent) {
        close(pEvent->afd_[0]);
        close(pEvent->afd_[1]);
        CriticalSection_Destroy(&pEvent->crit_);
    }
}

// WakeUp
void Event_WakeUp(tEvent *pEvent)
{
    //printf("Event_WakeUp\n");
    if(pEvent) {
        CriticalSection_Enter(&pEvent->crit_);
        if (!pEvent->fSignaled_) {
            const unsigned char b[1] = { 0 };
            if (VERIFY(1 == write(pEvent->afd_[1], b, sizeof(b)))) {
                pEvent->fSignaled_ = true;
            }
        }
        CriticalSection_Leave(&pEvent->crit_);
    }
}

void Event_OnPreEvent(tEvent *pEvent, uint32_t ff) {
    // It is not possible to perfectly emulate an auto-resetting event with
    // pipes.  This simulates it by resetting before the event is handled.

    CriticalSection_Enter(&pEvent->crit_);
    if (pEvent->fSignaled_) {
        uint8_t b[4];  // Allow for reading more than 1 byte, but expect 1.
        VERIFY(1 == read(pEvent->afd_[0], b, sizeof(b)));
        pEvent->fSignaled_ = false;
    }
    CriticalSection_Leave(&pEvent->crit_);
}

void Event_OnEvent(tEvent *pEvent, uint32_t ff, int err) {
    ASSERT(false);
}
    
bool Event_Wait(tEvent *pEvent, int cmsWait, bool process_io)
{
    //printf("Event_Wait\n");
    static vEventWaitCnt=0;
    // Calculate timing information
    if(!pEvent)
        return false;
    
    vEventWaitCnt++;
    
    struct timeval *ptvWait = NULL;
    struct timeval tvWait;
    struct timeval tvStop;
    if (cmsWait != kForever) {
        // Calculate wait timeval
        tvWait.tv_sec = cmsWait / 1000;
        tvWait.tv_usec = (cmsWait % 1000) * 1000;
        ptvWait = &tvWait;
        
        // Calculate when to return in a timeval
        gettimeofday(&tvStop, NULL);
        tvStop.tv_sec += tvWait.tv_sec;
        tvStop.tv_usec += tvWait.tv_usec;
        if (tvStop.tv_usec >= 1000000) {
            tvStop.tv_usec -= 1000000;
            tvStop.tv_sec += 1;
        }
    }
    
    // Zero all fd_sets. Don't need to do this inside the loop since
    // select() zeros the descriptors not signaled
    
    fd_set fdsRead;
    FD_ZERO(&fdsRead);
    fd_set fdsWrite;
    FD_ZERO(&fdsWrite);
    
    fWait_ = true;
    
    while (fWait_) {

        int fdmax = -1;
        
        //CritScope cr(&crit_);
        CriticalSection_Enter(&pEvent->crit_);
        {
            //if (!process_io && (pdispatcher != signal_wakeup_))
            if (!process_io)
                continue;
            
            int fd = pEvent->afd_[0];
            if (fd > fdmax)
                fdmax = fd;
            FD_SET(fd, &fdsRead);
        }
        CriticalSection_Leave(&pEvent->crit_);
        
        // Wait then call handlers as appropriate
        // < 0 means error
        // 0 means timeout
        // > 0 means count of descriptors ready
        int n = select(fdmax + 1, &fdsRead, &fdsWrite, NULL, ptvWait);
        
        // If error, return error.
        //printf("time=%ld:%d n=%d vEventWaitCnt=%d\n", tvStop.tv_sec, tvStop.tv_usec, n, vEventWaitCnt);
        //printf("n=%d vEventWaitCnt=%d\n", n, vEventWaitCnt);
        if (n < 0) {
            if (errno != EINTR) {
                LOG_F(LS_ERROR, "select");
                return false;
            }
            // Else ignore the error and keep going. If this EINTR was for one of the
            // signals managed by this PhysicalSocketServer, the
            // PosixSignalDeliveryDispatcher will be in the signaled state in the next
            // iteration.
        } else if (n == 0) {
            // If timeout, return success
            return true;
        } else {
            // We have signaled descriptors
            CriticalSection_Enter(&pEvent->crit_);
            {
                int fd = pEvent->afd_[0];
                uint32_t ff = 0;
                int errcode = 0;
                
                if (FD_ISSET(fd, &fdsRead) || FD_ISSET(fd, &fdsWrite)) {
                    socklen_t len = sizeof(errcode);
                    //int	getsockopt(int, int, int, void * __restrict, socklen_t * __restrict);
                    
                    getsockopt(fd, SOL_SOCKET, SO_ERROR, &errcode, &len);
                }
                
                if (FD_ISSET(fd, &fdsRead)) {
                    FD_CLR(fd, &fdsRead);
                    if (errcode) {
                        ff |= DE_CLOSE;
                    } else {
                        ff |= DE_READ;
                    }
                }
                
                // Tell the descriptor about the event.
                if (ff != 0) {
                    Event_OnPreEvent(pEvent, ff);
                    //Event_OnEvent(pEvent, ff, errcode);
                    fWait_ = false;
                }
            }
            CriticalSection_Leave(&pEvent->crit_);
        }
        
        // Recalc the time remaining to wait. Doing it here means it doesn't get
        // calced twice the first time through the loop
        if (ptvWait) {
            ptvWait->tv_sec = 0;
            ptvWait->tv_usec = 0;
            struct timeval tvT;
            gettimeofday(&tvT, NULL);
            if ((tvStop.tv_sec > tvT.tv_sec)
                || ((tvStop.tv_sec == tvT.tv_sec)
                    && (tvStop.tv_usec > tvT.tv_usec))) {
                    ptvWait->tv_sec = tvStop.tv_sec - tvT.tv_sec;
                    ptvWait->tv_usec = tvStop.tv_usec - tvT.tv_usec;
                    if (ptvWait->tv_usec < 0) {
                        ASSERT(ptvWait->tv_sec > 0);
                        ptvWait->tv_usec += 1000000;
                        ptvWait->tv_sec -= 1;
                    }
                }
        }
    }
    
    return true;
}

    
// ==== Thread ====
// rtc::Thread::Current()->ProcessMessages()
// rtc::Thread::Current()->Clear()
// rtc::Thread::Current()->Post()
// rtc::Thread::Current()->PostDelayed()
// rtc::Thread::Current()->size()

    
bool Thread_ProcessMessages(tMessageQueue *pIn, int cmsLoop)
{
    U32 msEnd = (kForever == cmsLoop) ? 0 : TimeAfter(cmsLoop);
    int cmsNext = cmsLoop;
    
    while (true) {
        {
            tMessage msg;
            memset(&msg, 0, sizeof(tMessage));
            
            //LOG_F(LS_INFO, "MQueue_Get");
            if (!MQueue_Get(pIn, &msg, cmsNext, true)) {
                //printf( "return !IsQuitting %d  ", !IsQuitting(pIn));
                return !IsQuitting(pIn);
            }
            
            //MQueue_Dispatch(&msg);
            if(pIn->pOnMessageCB)
                pIn->pOnMessageCB(&msg);
            
            if (cmsLoop != kForever) {
                cmsNext = TimeUntil(msEnd);
                if (cmsNext < 0)
                    return true;
            }
            //printf("cmsNext=%d\n",cmsNext);
        }
    }
}


#if 0
void Thread_Clear(MessageHandler *phandler, uint32 id,
                  MessageList* removed) {
    CritScope cs(&crit_);
    
    // Remove messages on sendlist_ with phandler
    // Object target cleared: remove from send list, wakeup/set ready
    // if sender not NULL.
    
    std::list<_SendMessage>::iterator iter = sendlist_.begin();
    while (iter != sendlist_.end()) {
        _SendMessage smsg = *iter;
        if (smsg.msg.Match(phandler, id)) {
            if (removed) {
                removed->push_back(smsg.msg);
            } else {
                delete smsg.msg.pdata;
            }
            iter = sendlist_.erase(iter);
            *smsg.ready = true;
            smsg.thread->socketserver()->WakeUp();
            continue;
        }
        ++iter;
    }
    
    MQueue_Clear(phandler, id, removed);
    //MQueue_Clear(tMessageQueue *pMQueue, id, tMI_DLIST* removed);
}
#endif
/*****************************************************************************/

#ifdef __cplusplus
}
#endif

/**  @} */
/******************************************************************************
 *  CONFIDENTIAL
 *****************************************************************************/
