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
   if(pDList)
   {
      tMI_DLNODE *vpNode = MI_DlFirst(pDList);
      if(vpNode)
      {
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
   if(pDList)
   {
      tMI_DLNODE *vpNode = MI_DlLast(pDList);
      if(vpNode)
      {
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
   if(pDList)
   {
      if(MI_DlCount(pDList)==0)
      {
         return true;
      }
   }
   return false;
}

// Return size
U32 MList_Size(tMI_DLIST *pDList)
{
   return MI_DlCount(pDList);
}


// Returns a reference to the first element in the list container.
tMessage *MList_Front(tMI_DLIST *pDList)
{
   if(pDList)
   {
      tMI_DLNODE *vpNode = MI_DlFirst(pDList);
      if(vpNode)
      {
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
   if(pDList)
   {
      tMI_DLNODE *vpNode = MI_DlLast(pDList);
      if(vpNode)
      {
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
   if(pDList)
   {
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

    
void PQueue_Dump(tMI_PQUEUE *pQ) {

    tMI_PQNODE *vpTest = pQ->head;
    tMI_PQNODE *vpVNode = pQ->head;
    if(pQ->head)
        printf("Dump PQueue (pQ->head->v.next=0x%08x) (pQ->tail->v.previous=0x%08x)\n", (U32)pQ->head->v.next, (U32)pQ->tail->v.previous);
    while(vpTest) {
        printf(" 0x%08x ", (U32)vpTest);
        if(vpTest->h.next!=0) {
            vpTest = vpTest->h.next;
        }
        else {
            printf(" \n");
            vpTest = vpVNode->v.next;
            vpVNode = vpVNode->v.next;
        }
    }
    printf("\n");
}
    
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
   if(pPQueue)
   {
      U32 vCount = MI_PQCount(pPQueue);
      if((vCount==0) ||(vCount==MI_UTIL_ERROR))
      {
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
   printf("PQueue_Pop() head=0x%08x tail=0x%08x node:0x%08x count=%d\n", (U32)pQ->head, (U32)pQ->tail, (U32)pPQNode, (U32)pQ->count);
    PQueue_Dump(pQ);
   if(pPQNode)
   {
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
   printf("PQueue_Push() head=0x%08x tail=0x%08x node:0x%08x count=%d\n", (U32)pQ->head, (U32)pQ->tail, (U32)pPQNode, (U32)pQ->count);
   PQueue_Dump(pQ);
}

tDelayMessage *PQueue_Top(tMI_PQUEUE *pQ)
{
   if(pQ)
   {
      tMI_PQNODE *pPQNode = pQ->head;
      printf("PQueue_Top() head=0x%08x tail=0x%08x node:0x%08x\n", (U32)pQ->head, (U32)pQ->tail, (U32)pPQNode);
      if(pPQNode)
      {
         tDelayMessage *vpTxt;
         vpTxt = MI_NODEENTRY(pPQNode, tDelayMessage, PQNode);
         return vpTxt;      
      }
   }
   return NULL;
}

tMI_PQNODE * PQueue_Erase(tMI_PQUEUE *pQ, tMI_PQNODE *pNode) 
{
   printf("PQueue_Erase() count=%d head=0x%08x tail=0x%08x remove=0x%08x\n", pQ->count, (U32)pQ->head, (U32)pQ->tail, (U32)pNode);
   if (pQ && pNode) {
      if (pQ->head == 0) {
         return NULL;
      }
      else {
         tMI_PQNODE *scan = pQ->head;
         S32 priority = pQ->priority(pNode);
          
         while (scan) {
            // remove node
            if(pNode == scan) {
               tMI_PQNODE *vpN = scan;
               
                // remove head
                if (pQ->head == vpN) {
                    if (vpN->h.next == 0) {
                        pQ->head = vpN->v.next;
                        if (vpN->v.next == 0) {
                            pQ->tail = 0;
                        }
                        else {
                            vpN->v.next->v.previous = vpN->v.previous;
                        }
                    }
                    else {
                        pQ->head = vpN->h.next;
                        vpN->h.next->v.previous = 0;
                        vpN->h.next->v.next = vpN->v.next;
                        
                        if (vpN->h.previous == vpN->h.next ) {
                            vpN->h.next->h.previous = 0;
                        }
                        else {
                            vpN->h.next->h.previous = vpN->h.previous;
                        }
                        
                        if (vpN->v.next == 0) {
                            pQ->tail = vpN->h.next;
                        }
                        else {
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
                        }
                        else {
                            vpN->v.previous->v.next = 0;
                            //vpN->v.previous->h.next = vpN->h.next;
                            
                            //vpN->v.previous->v.next = vpN->v.next;
                        }
                    }
                    else {
                        printf("vpN->h.previous != 0\n");
                        pQ->tail = vpN->v.previous;
                        if (vpN->v.previous != 0) {
                           vpN->v.previous->v.next = vpN->v.next;
                        }
                        else {
                          // The node is head, should be handle in above case
                          ASSERT(0);
                        }
                    }
                }
                // remove items not head and tail
                else {
                    if (vpN->h.next == 0) {
                        if (vpN->v.next == 0) {
                            if(vpN->h.previous) {
                                vpN->h.previous->h.next = vpN->h.next;
                            }
                            else {
                                vpN->v.previous->v.next = vpN->v.next;
                            }
                        }
                        else {
                            if (vpN->h.previous == 0) {
                                if (vpN->v.previous == 0) {
                                    // This is head node
                                    ASSERT(pQ->head == vpN);
                                }
                                else {
                                    // remove item 2
                                    //     item1
                                    //     item2
                                    //     item3
                                    vpN->v.previous->v.next = vpN->v.next;
                                    vpN->v.next->v.previous = vpN->v.previous;
                                }
                            }
                            else {
                                // remove item 2
                                //     item1
                                //     item2
                                //     item3
                                vpN->v.previous->v.next = vpN->v.next;
                                vpN->v.next->v.previous = vpN->v.previous;
                                //vpN->h.previous->h.next = vpN->h.next;
//                                if (vpN->v.previous == 0) {
//                                    // do nothing;
//                                    ASSERT(vpN->v.previous == 0);
//                                }
//                                else {
//                                    vpN->v.previous->v.next = vpN->v.next;
//                                    vpN->v.next->v.previous = vpN->v.previous;
//                                }
                            }
                        }
                    }
                    else {
                        if (vpN->v.next == 0) {
                            if (vpN->h.previous == 0) {
                                if (vpN->v.previous == 0) {
                                    // This is head node
                                    ASSERT(pQ->head == vpN);
                                }
                                else {
                                    vpN->v.previous->v.next = vpN->h.next;
                                }
                            }
                            else {
                                vpN->h.previous->h.next = vpN->h.next;
                                vpN->h.next->h.previous = vpN->h.previous;
                                if (vpN->v.previous == 0) {
                                    // do nothing
                                }
                                else {
                                    ASSERT(vpN->v.previous != 0);
                                }
                            }
                        }
                        else {
                            if (vpN->h.previous == 0) {
                                if (vpN->v.previous == 0) {
                                    // This is head node
                                    ASSERT(pQ->head == vpN);
                                }
                                else {
                                    vpN->h.next->h.previous = 0;
                                    vpN->v.previous->v.next = vpN->h.next;
                                    vpN->v.next->v.previous = vpN->h.next;
//                                    vpN->v.previous->v.next = vpN->v.next;
//                                    vpN->v.next->v.previous = vpN->v.previous;
                                }
                            }
                            else {
                                if (vpN->v.previous == 0) {
                                    ASSERT(vpN->v.previous == 0);
                                }
                                else {
                                    // remove item2
                                    //     item1
                                    //     item2  item3
                                    //     item4
                                    
                                    //vpN->h.next->h.previous = 0;
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
                
                
               pQ->count--;
               printf("PQueue_Erase() after erase, count=%d head=0x%08x tail=0x%08x\n", pQ->count, (U32)pQ->head, (U32)pQ->tail);
               
               PQueue_Dump(pQ);
                
               if (vpN->h.next == 0)
                  return vpN->v.next;
               else
                  return vpN->h.next;
                  
            } 
            else {
               S32 thisprio = pQ->priority(scan);
                
                if (priority == thisprio) {
                   scan = scan->h.next;
                }
                else {
                   scan = scan->v.next;
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
void* PreRun(void* pv) { 
   tMessageQueue *vpIn = (tMessageQueue *)pv;
   vpIn->fStop = false;
   MQueue_ProcessMessages(vpIn, kForever);
   return NULL;
}

tMessageQueue * MQueue_Init(tOnMessageCB pCB) {
   tMessageQueue *vpIn = malloc(sizeof(tMessageQueue));
   if(vpIn)
   {
      memset(vpIn, 0, sizeof(tMessageQueue));
      vpIn->fStop = false;
      vpIn->fPeekKeep = false;
      memset(&vpIn->msgPeek, 0, sizeof(tMessage));
      MList_Init(&vpIn->msgq);
      PQueue_Init(&vpIn->dmsgq);
      vpIn->dmsgq_next_num = 0;
      
      Event_Init(false, false);
      vpIn->pOnMessageCB = pCB;

      // create a thread to handle message received (pass the msg to pOnMessageCB())
      pthread_mutex_init(&_mutex, NULL);
      pthread_cond_init(&_cond, NULL);
      pthread_attr_t attr;
      pthread_attr_init(&attr);         
      pthread_create(&vpIn->thread, &attr, PreRun, vpIn);
      pthread_attr_destroy(&attr);
      
      return vpIn;      
   }
   return NULL;
}

void MQueue_Destroy(tMessageQueue *vpIn) {
   // TODO: send signal the MQueue thread to destroy MQueue
   
   if(vpIn)
   {
      //MList_Init(&vpIn->msgq);
      //PQueue_Init(&vpIn->dmsgq);
      
      pthread_mutex_destroy(&_mutex);
      pthread_cond_destroy(&_cond);
      
//#if 1
      pthread_exit(NULL);
//#else
//      int vReturn;
//      void *res;
//      vReturn = pthread_join(&vpIn->thread, &res);
//#endif
       
      Event_Destroy();
      
      free(vpIn);
   }
}


void MQueue_Clear(tMessageQueue *pMQueue, U32 id, tMI_DLIST* removed) {
   //CritScope cs(&crit_);
   //LOG(LS_INFO, "pthread_mutex_lock _mutex");
   pthread_mutex_lock(&_mutex);
   LOG(LS_INFO, "MQueue_Clear");
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
         }
      }
      pMQueue->fPeekKeep = false;
   }

   // Remove from ordered message queue
   
   tMessage *pMsg = MList_Begin(&pMQueue->msgq);
   if(pMsg!=NULL)
   {
      tMI_DLNODE *pDLNode = &pMsg->DLNode;
      for (; pDLNode != NULL;) {
         printf("pMQueue:: pMsg->message_id=%d, id=%d\n", pMsg->message_id, id);         
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
               }
            }
            //it = msgq_.erase(it);
            pDLNode = MList_Erase(&pMQueue->msgq, pDLNode);         
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
   if(pDelayMsg!=NULL)
   {
      tMI_PQNODE *pPQNode = &pDelayMsg->PQNode;  
      
      if(pPQNode) {
         for(; pPQNode != NULL ; ) {
            //LOG(LS_INFO, "pPQNode != NULL");
            pDelayMsg = MI_NODEENTRY(pPQNode, tDelayMessage, PQNode);  
            //if(pDelayMsg)
            {
               pMsg = pDelayMsg->pMsg;
               //if (it->msg_.Match(phandler, id)) {
               //printf("PQueue:: pMsg->message_id=%d, id=%d\n", pMsg->message_id, id);
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
                  }
                  
                  // remove item in queue
                  pPQNode = PQueue_Erase(&pMQueue->dmsgq, pPQNode);         
               } else {
                  //*new_end++ = *it;
                  if (pPQNode->h.next == 0)
                  {
                     pPQNode = pPQNode->v.next;
                  }
                  else
                  {
                     pPQNode = pPQNode->h.next;         
                  }
                  //printf("PQueue:: pPQNode=0x%08x\n", (U32)pPQNode);
               }
            }
         }
      }
   }
   
   //dmsgq_.container().erase(new_end, dmsgq_.container().end());
   //dmsgq_.reheap();
   //LOG(LS_INFO, "pthread_mutex_unlock _mutex");
   pthread_mutex_unlock(&_mutex);   
  
}



bool MQueue_ProcessMessages(tMessageQueue *pIn, int cmsLoop) {
   U32 msEnd = (kForever == cmsLoop) ? 0 : TimeAfter(cmsLoop);
   int cmsNext = cmsLoop;

   while (true) {
      tMessage msg;
      //LOG(LS_INFO, "MQueue_Get");
      if (!MQueue_Get(pIn, &msg, cmsNext, true))
      {
         //LOG(LS_INFO, "return");
         return !IsQuitting(pIn);
      }

      if(pIn->pOnMessageCB)
         pIn->pOnMessageCB(&msg);

      if (cmsLoop != kForever) {
         cmsNext = TimeUntil(msEnd);
         if (cmsNext < 0)
            return true;
      }
   }
}

void MQueue_Post(tMessageQueue *pIn, tMessageHandler *phandler, U32 id, tMessageData *pData, bool time_sensitive) {
   if (pIn->fStop)
      return;

   // Keep thread safe
   // Add to the priority queue. Gets sorted soonest first.
   // Signal for the multiplexer to return.

   //CritScope cs(&crit_);
   LOG(LS_INFO, "pthread_mutex_lock _mutex");
   pthread_mutex_lock(&_mutex);
   tMessage *pMsg = malloc(sizeof(tMessage));
   pMsg->phandler = phandler;
   pMsg->message_id = id;
   pMsg->pData = pData;
   if (time_sensitive) {
      pMsg->ts_sensitive = Time() + kMaxMsgLatency;
   }
   //msgq_.push_back(msg);
   MList_Push_Back(&pIn->msgq, &pMsg->DLNode);

   //ss_->WakeUp();
   Event_Set();
   LOG(LS_INFO, "pthread_mutex_unlock _mutex"); 
   pthread_mutex_unlock(&_mutex); 
}

  
void MQueue_DoDelayPost(tMessageQueue *pIn, int cmsDelay, U32 tstamp,
   tMessageHandler *phandler, U32 id, tMessageData *pData) {
    
   ASSERT(pIn != NULL);
      
   if (pIn->fStop)
      return;

   // Keep thread safe
   // Add to the priority queue. Gets sorted soonest first.
   // Signal for the multiplexer to return.

   //CritScope cs(&crit_);
   //LOG(LS_INFO, "pthread_mutex_lock _mutex");
   pthread_mutex_lock(&_mutex);
   tMessage *pMsg = malloc(sizeof(tMessage));
   if(pMsg) {
      pMsg->phandler = phandler;
      pMsg->message_id = id;
      pMsg->pData = pData;
      
      tDelayMessage  *pDMsg = malloc(sizeof(tDelayMessage));
      if(pDMsg) {      
         pDMsg->cmsDelay  = cmsDelay;
         pDMsg->msTrigger  = tstamp;
         pDMsg->num  = pIn->dmsgq_next_num;
         pDMsg->pMsg  = pMsg;

         //dmsgq_.push(dmsg);   
         LOG(LS_INFO, "PQueue_Push");
         //printf("&pIn->dmsgq=0x%x pDMsg=0x%x pDMsg->PQNode=0x%x\n", (U32)&pIn->dmsgq, (U32)pDMsg, (U32)&pDMsg->PQNode);
         PQueue_Push(&pIn->dmsgq, &pDMsg->PQNode);
      }
      else
      {
         free(pMsg);
      }
   }
   
   // If this message queue processes 1 message every millisecond for 50 days,
   // we will wrap this number.  Even then, only messages with identical times
   // will be misordered, and then only briefly.  This is probably ok.
   //VERIFY(0 != ++dmsgq_next_num_);
   //ss_->WakeUp();
   Event_Set();
   //LOG(LS_INFO, "pthread_mutex_unlock _mutex");
   pthread_mutex_unlock(&_mutex); 
}

void MQueue_PostDelayed(tMessageQueue *pIn, int cmsDelay, U32 id, tMessageData *pData) {
   return MQueue_DoDelayPost(pIn, cmsDelay, TimeAfter(cmsDelay), NULL, id, pData);
}

void MQueue_PostAt(tMessageQueue *pIn, U32 tstamp, tMessageHandler *phandler, U32 id) {
   return MQueue_DoDelayPost(pIn, TimeUntil(tstamp), tstamp, phandler, id, NULL);
}
    
void MQueue_Quit(tMessageQueue *pIn) {
   LOG(LS_VERBOSE, "MQueue_Quit");
   pIn->fStop = true;
   //ss_->WakeUp();
   Event_Set();
}

void MQueue_Restart(tMessageQueue *pIn) {
   LOG(LS_VERBOSE, "MQueue_Restart");
   pIn->fStop = false;
}


bool IsQuitting(tMessageQueue *pIn) {
  return pIn->fStop;
}

size_t MQueue_Size(tMessageQueue *pIn) {
   //CritScope cs(&crit_);  // msgq_.size() is not thread safe.
   LOG(LS_INFO, "pthread_mutex_lock _mutex");
   pthread_mutex_lock(&_mutex);
   int i;
   i = MList_Size(&pIn->msgq) + PQueue_Size(&pIn->dmsgq) + (pIn->fPeekKeep ? 1u : 0u);
   LOG(LS_INFO, "pthread_mutex_unlock _mutex"); 
   pthread_mutex_unlock(&_mutex);   
   return i;
}

bool MQueue_Empty(tMessageQueue *pIn) { return MQueue_Size(pIn) == 0u; }

bool MQueue_Peek(tMessageQueue *pIn, tMessage *pmsg, int cmsWait) {
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

bool MQueue_Get(tMessageQueue *pIn, tMessage *pmsg, int cmsWait, bool process_io) {
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
      //ReceiveSends();

      // Check for posted events
      int cmsDelayNext = kForever;
      bool first_pass = true;
      while (true) {
         // All queue operations need to be locked, but nothing else in this loop
         // (specifically handling disposed message) can happen inside the crit.
         // Otherwise, disposed MessageHandlers will cause deadlocks.
         {
            //CritScope cs(&crit_);
            //LOG(LS_INFO, "pthread_mutex_lock _mutex");
            pthread_mutex_lock(&_mutex);

            // On the first pass, check for delayed messages that have been
            // triggered and calculate the next trigger time.
            if (first_pass) {
               first_pass = false;
               while (!PQueue_Empty(&pIn->dmsgq)) {
                  tDelayMessage *pDelayMsg = PQueue_Top(&pIn->dmsgq);
                  //if(pDelayMsg) {
                  {
                     if (TimeIsLater(msCurrent, pDelayMsg->msTrigger)) {
                        cmsDelayNext = TimeDiff(pDelayMsg->msTrigger, msCurrent);
                        break;
                     }
                     //printf("__LINE__ : %d 0x%x\n", __LINE__, (U32)&pIn->dmsgq);
                     MList_Push_Back(&pIn->msgq, &(pDelayMsg->pMsg->DLNode));
                     PQueue_Pop(&pIn->dmsgq);
                  }
               }
            }
            // Pull a message off the message queue, if available.
            if (MList_Empty(&pIn->msgq)) {
               //LOG(LS_INFO, "pthread_mutex_unlock _mutex"); 
               pthread_mutex_unlock(&_mutex);
               break;
            } else {
               tMessage *pTmpMsg = MList_Front(&pIn->msgq);
               *pmsg = *pTmpMsg;
               MList_Pop_Front(&pIn->msgq);
            }
            //LOG(LS_INFO, "pthread_mutex_unlock _mutex"); 
            pthread_mutex_unlock(&_mutex);   
         }  // crit_ is released here.

         // Log a warning for time-sensitive messages that we're late to deliver.
         if (pmsg->ts_sensitive) {
            S32 delay = TimeDiff(msCurrent, pmsg->ts_sensitive);
            if (delay > 0) {
               char pTmp[1024] = {0};
               sprintf(pTmp, "id: %d  delay: %d  ms\n", pmsg->message_id, (delay + kMaxMsgLatency));
               LOG(LS_VERBOSE, pTmp);
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
            }
            //*pmsg = Message();            
            memset(pmsg, 0, sizeof(*pmsg));
            continue;
         }
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
      if(!Event_Wait(cmsNext))
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
void Event_Init(bool manual_reset, bool initially_signaled) {
   is_manual_reset_ = manual_reset;
   event_status_ = initially_signaled;
   VERIFY(pthread_mutex_init(&event_mutex_, NULL) == 0);
   VERIFY(pthread_cond_init(&event_cond_, NULL) == 0);
}

void Event_Destroy(void) {
   pthread_mutex_destroy(&event_mutex_);
   pthread_cond_destroy(&event_cond_);
}

void Event_Set(void) {
   //LOG(LS_INFO, "Event_Set  pthread_mutex_lock  event_mutex_");
   pthread_mutex_lock(&event_mutex_);
   event_status_ = true;
   pthread_cond_broadcast(&event_cond_);
   //LOG(LS_INFO, "Event_Set  pthread_mutex_unlock  event_mutex_");   
   pthread_mutex_unlock(&event_mutex_);
}

void Event_Reset(void) {
   LOG(LS_INFO, "pthread_mutex_lock  event_mutex_");
   pthread_mutex_lock(&event_mutex_);
   event_status_ = false;
   LOG(LS_INFO, "pthread_mutex_unlock  event_mutex_"); 
   pthread_mutex_unlock(&event_mutex_);
}

bool Event_Wait(int cms) {

   //LOG(LS_INFO, "pthread_mutex_lock  event_mutex_");    
   pthread_mutex_lock(&event_mutex_);
   //printf("Event_Wait for %d ms\n", cms);

   int error = 0;

   if (cms != kForever) {
   // Converting from seconds and microseconds (1e-6) plus
   // milliseconds (1e-3) to seconds and nanoseconds (1e-9).

      struct timespec ts;
#if HAVE_PTHREAD_COND_TIMEDWAIT_RELATIVE
      // Use relative time version, which tends to be more efficient for
      // pthread implementations where provided (like on Android).
      ts.tv_sec = cms / 1000;
      ts.tv_nsec = (cms % 1000) * 1000000;
#else
      struct timeval tv;
      gettimeofday(&tv, NULL);

      ts.tv_sec = tv.tv_sec + (cms / 1000);
      ts.tv_nsec = tv.tv_usec * 1000 + (cms % 1000) * 1000000;

      // Handle overflow.
      if (ts.tv_nsec >= 1000000000) {
         ts.tv_sec++;
         ts.tv_nsec -= 1000000000;
      }
#endif

      while (!event_status_ && error == 0) {
#if HAVE_PTHREAD_COND_TIMEDWAIT_RELATIVE
         error = pthread_cond_timedwait_relative_np(
             &event_cond_, &event_mutex_, &ts);
#else
         error = pthread_cond_timedwait(&event_cond_, &event_mutex_, &ts);
#endif
      }
   } else {
      while (!event_status_ && error == 0)
         error = pthread_cond_wait(&event_cond_, &event_mutex_);
   }

   // NOTE(liulk): Exactly one thread will auto-reset this event. All
   // the other threads will think it's unsignaled.  This seems to be
   // consistent with auto-reset events in WIN32.
   if (error == 0 && !is_manual_reset_)
      event_status_ = false;

   //LOG(LS_INFO, "pthread_mutex_unlock  event_mutex_");      
   pthread_mutex_unlock(&event_mutex_);

   return (error == 0);
}

/*****************************************************************************/

#ifdef __cplusplus
}
#endif

/**  @} */
/******************************************************************************
 *  CONFIDENTIAL
 *****************************************************************************/