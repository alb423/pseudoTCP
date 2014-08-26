/**
 * @file    fifo_buffer.c
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
#include "fifo_buffer.h"
#include "pseudo_tcp_porting.h"
/*** MACROS ******************************************************************/


/*** GLOBAL VARIABLE DECLARATIONS (EXTERN) ***********************************/


/*** PRIVATE TYPES DEFINITIONS ***********************************************/


/*** PRIVATE VARIABLE DECLARATIONS (STATIC) **********************************/

/*** PRIVATE FUNCTION PROTOTYPES *********************************************/
// Helper method that implements ReadOffset. Caller must acquire a lock
// when calling this method.
tStreamResult ReadOffsetLocked(tFIFO_BUFFER *pFifo,
                              void* buffer,
                              size_t bytes,
                              size_t offset,
                              size_t* bytes_read);

// Helper method that implements WriteOffset. Caller must acquire a lock
// when calling this method.
tStreamResult WriteOffsetLocked(tFIFO_BUFFER *pFifo,
                               const void* buffer,
                               size_t bytes,
                               size_t offset,
                               size_t* bytes_written);

/*** PUBLIC FUNCTION DEFINITIONS *********************************************/


///////////////////////////////////////////////////////////////////////////////
/// @par Declaration
///   Creates a FIFO buffer with the specified capacity.<br>
///
/// @param length
///   The size of the FIFO buffer.<br>
///
/// @return
///   None:         return with pointer of tFIFO_BUFFER.<br>
///
/// @par Conditions
///   None.<br>
///
/// @par Usage
///   None.<br>
///
///////////////////////////////////////////////////////////////////////////////
tFIFO_BUFFER * FIFO_Create(size_t vSize, tFIFO_CB pReadCB, tFIFO_CB pWriteCB)
{
   // TODO: implement a lock mechanism to lock related operation
   tFIFO_BUFFER *vpPtr;
   if(vSize)
   {
      vpPtr = malloc(sizeof(tFIFO_BUFFER));
      //vpPtr = calloc(sizeof(tFIFO_BUFFER), sizeof(U8));
      if(vpPtr)
      {
         memset(vpPtr, 0, sizeof(tFIFO_BUFFER));
         vpPtr->buffer = malloc(vSize);
         if(!vpPtr->buffer)
         {
            free(vpPtr);
            return NULL;
         }
         memset(vpPtr->buffer, 0, vSize);
          
         vpPtr->buffer_length = vSize;
         vpPtr->state = SS_OPEN;
         vpPtr->data_length = 0;
         vpPtr->read_position = 0;
         vpPtr->pNotifyReadCB = pReadCB;
         vpPtr->pNotifyWriteCB = pWriteCB;          
         
//         pthread_mutexattr_t mutex_attribute;
//         pthread_mutexattr_init(&mutex_attribute);
//         pthread_mutexattr_settype(&mutex_attribute, PTHREAD_MUTEX_RECURSIVE);
//         pthread_mutex_init(&vpPtr->mutex, &mutex_attribute);
//         pthread_mutexattr_destroy(&mutex_attribute);
          
         pthread_mutexattr_t mutex_attribute;
         pthread_mutexattr_init(&mutex_attribute);
         pthread_mutexattr_settype(&mutex_attribute, PTHREAD_MUTEX_ERRORCHECK);
         pthread_mutex_init(&vpPtr->mutex, &mutex_attribute);
         pthread_mutexattr_destroy(&mutex_attribute);
          
//          pthread_mutex_init(&vpPtr->mutex, NULL);
      }
      return vpPtr;
   }
   return NULL;
}

///////////////////////////////////////////////////////////////////////////////
/// @par Declaration
///   Destroy a FIFO buffer.<br>
///
/// @param pFifo
///   The pointer of tFIFO_BUFFER.<br>
///
/// @return
///   None:        <br>
///
/// @par Conditions
///   None.<br>
///
/// @par Usage
///   None.<br>
///
///////////////////////////////////////////////////////////////////////////////
void FIFO_Destroy(tFIFO_BUFFER *pFifo)
{
   if(pFifo)
   {
      if(pFifo->buffer)
      {
         free(pFifo->buffer);
         return;
      }
      pthread_mutex_destroy(&pFifo->mutex);
      free(pFifo);
   }
}


///////////////////////////////////////////////////////////////////////////////
/// @par Declaration
///   Gets the amount of data currently readable from the buffer.<br>
///
/// @param pFifo
///   The pointer of tFIFO_BUFFER.<br>
///
/// @param size
///   The size of buffer in tFIFO_BUFFER.<br>
///
/// @return
///   None:        <br>
///
/// @par Conditions
///   None.<br>
///
/// @par Usage
///   None.<br>
///
///////////////////////////////////////////////////////////////////////////////
bool FIFO_GetBuffered(tFIFO_BUFFER *pFifo, size_t* size)
{
  //CritScope cs(&crit_);

   if(!pFifo) {
      return false;
   }

   pthread_mutex_lock(&pFifo->mutex);
   *size = pFifo->data_length;
   pthread_mutex_unlock(&pFifo->mutex);
   
   return true;
}


///////////////////////////////////////////////////////////////////////////////
/// @par Declaration
///   Gets stream state.<br>
///
/// @param pFifo
///   The pointer of tFIFO_BUFFER.<br>
///
/// @return       The tStreamState(SS_CLOSED, SS_OPENING, SS_OPEN).
///   None:        <br>
///
/// @par Conditions
///   None.<br>
///
/// @par Usage
///   None.<br>
///
///////////////////////////////////////////////////////////////////////////////
tStreamState FIFO_GetState(tFIFO_BUFFER *pFifo) 
{
   if(!pFifo)
      return SS_CLOSED;  
   return pFifo->state;
}


///////////////////////////////////////////////////////////////////////////////
/// @par Declaration
///   Resizes the buffer to the specified capacity. Fails if data_length_ > size. <br>
///
/// @param pFifo
///   The pointer of tFIFO_BUFFER.<br>
///
/// @param size
///   The size of fifo capacity.<br>
///
/// @return
///   None:         return true/false.<br>
///
/// @par Conditions
///   None.<br>
///
/// @par Usage
///   None.<br>
///
///////////////////////////////////////////////////////////////////////////////
bool FIFO_SetCapacity(tFIFO_BUFFER *pFifo, size_t size)
{
   // CritScope cs(&crit_);

   if(!pFifo) {
      return false;
   }

   pthread_mutex_lock(&pFifo->mutex);
   if (pFifo->data_length > size)
   {
      // don't need change
      pthread_mutex_unlock(&pFifo->mutex);
      return false;
   }

   if (size != pFifo->buffer_length) 
   {
      U8* vpBuffer = malloc(size);
      const size_t copy = pFifo->data_length;
      const size_t tail_copy = MIN(copy, pFifo->buffer_length - pFifo->read_position);
      memcpy(vpBuffer, &pFifo->buffer[pFifo->read_position], tail_copy);
      memcpy(vpBuffer + tail_copy, &pFifo->buffer[0], copy - tail_copy);
      
      //pFifo->buffer.reset(buffer);
      pFifo->buffer_length = size;
      if(pFifo->buffer)
         free(pFifo->buffer);
      pFifo->buffer = vpBuffer;
       
      pFifo->read_position = 0;
      pFifo->buffer_length = size;
   }
   pthread_mutex_unlock(&pFifo->mutex);   
   
   return true;
}


///////////////////////////////////////////////////////////////////////////////
/// @par Declaration
///  Read into |buffer| with an offset from the current read position, offset <br>
///  is specified in number of bytes. <br>
///  This method doesn't adjust read position nor the number of available <br>
///  bytes, user has to call ConsumeReadData() to do this. <br>
///
/// @param pFifo
///   The pointer of tFIFO_BUFFER.<br>
///
/// @param buffer
///   The buffer to place the read data.<br>
///
/// @param bytes
///   The size user want to read.<br>
///
/// @param offset
///   The offset to start read.<br>
///
/// @param bytes_read
///   The size of bytes read.<br>
///
/// @return
///   None:         return with tStreamResult.<br>
///
/// @par Conditions
///   None.<br>
///
/// @par Usage
///   None.<br>
///
///////////////////////////////////////////////////////////////////////////////
tStreamResult FIFO_ReadOffset(tFIFO_BUFFER *pFifo, 
                        void* buffer, 
                        size_t bytes,
                        size_t offset, 
                        size_t* bytes_read) 
{
   // CritScope cs(&crit_);

   tStreamResult vResult;
   if(!pFifo) {
      return SR_ERROR;
   }
   
   pthread_mutex_lock(&pFifo->mutex);
   vResult = ReadOffsetLocked(pFifo, buffer, bytes, offset, bytes_read);
    
#ifdef CHECK_COPY_BUFFER
    size_t i=0;
    if(bytes_read) {
        if(*bytes_read > 24+4) {
            i = *bytes_read-1;
            if(((char *)buffer)[i]==0x00) {
                ASSERT(((char *)buffer)[i]!=0x00);
            }
        }
    }
#endif
   pthread_mutex_unlock(&pFifo->mutex);
   
   return vResult;
}


///////////////////////////////////////////////////////////////////////////////
/// @par Declaration
///  Write |buffer| with an offset from the current write position, offset is <br>
///  specified in number of bytes. <br>
///  This method doesn't adjust the number of buffered bytes, user has to call <br>
///  ConsumeWriteBuffer() to do this. <br>
///
/// @param pFifo
///   The pointer of tFIFO_BUFFER.<br>
///
/// @param buffer
///   The data to write.<br>
///
/// @param bytes
///   The size user want to write.<br>
///
/// @param offset
///   The offset to start write.<br>
///
/// @param bytes_read
///   The size of bytes wrote.<br>
///
/// @return
///   None:         return with tStreamResult.<br>
///
/// @par Conditions
///   None.<br>
///
/// @par Usage
///   None.<br>
///
///////////////////////////////////////////////////////////////////////////////
tStreamResult FIFO_WriteOffset(tFIFO_BUFFER *pFifo,
                        const void* buffer, 
                        size_t bytes,
                        size_t offset, 
                        size_t* bytes_written) 
{
   // CritScope cs(&crit_);

   tStreamResult vResult;   
   if(!pFifo) {
      return SR_ERROR;
   }
   
   pthread_mutex_lock(&pFifo->mutex);
#ifdef CHECK_COPY_BUFFER
    size_t i=0;
    if(bytes > 24+4) {
        i = bytes-1;
        if(((char *)buffer)[i]==0x00) {
            ASSERT(((char *)buffer)[i]!=0x00);
        }
    }
#endif
    
   vResult = WriteOffsetLocked(pFifo, buffer, bytes, offset, bytes_written);
   pthread_mutex_unlock(&pFifo->mutex);   
   
   return vResult;
}



///////////////////////////////////////////////////////////////////////////////
/// @par Declaration
///  Read into |buffer| with an offset 0 from the current read position<br>
///  This method doesn't adjust read position nor the number of available <br>
///  bytes, user has to call ConsumeReadData() to do this. <br>
///
/// @param pFifo
///   The pointer of tFIFO_BUFFER.<br>
///
/// @param buffer
///   The buffer to place the read data.<br>
///
/// @param bytes
///   The size user want to read.<br>
///
/// @param bytes_read
///   The size of bytes read.<br>
///
/// @return
///   None:         return with tStreamResult.<br>
///
/// @par Conditions
///   None.<br>
///
/// @par Usage
///   None.<br>
///
///////////////////////////////////////////////////////////////////////////////
tStreamResult FIFO_Read(tFIFO_BUFFER *pFifo,
                  void* buffer, 
                  size_t bytes,
                  size_t* bytes_read)
{
   //CritScope cs(&crit_);

    
   bool was_writable = false;
    size_t copy = 0;
   
   tStreamResult result;
   if(!pFifo) {
      return SR_ERROR;
   }
    
   pthread_mutex_lock(&pFifo->mutex);
   was_writable = pFifo->data_length < pFifo->buffer_length;
    
   result = ReadOffsetLocked(pFifo, buffer, bytes, 0, &copy);
   if(result != SR_SUCCESS) {
       pthread_mutex_unlock(&pFifo->mutex);
       return result;
   }
    
    
#ifdef CHECK_COPY_BUFFER
    size_t i=0;
    if(copy > 24+4) {
        i = copy-1;
        if(((char *)buffer)[i]==0x00) {
            ASSERT(((char *)buffer)[i]!=0x00);
        }
    }
#endif
    
    
   if (result == SR_SUCCESS) {
      // If read was successful then adjust the read position and number of
      // bytes buffered.
      pFifo->read_position = (pFifo->read_position + copy) % pFifo->buffer_length;
      pFifo->data_length -= copy;
      if (bytes_read) 
      {
         *bytes_read = copy;
      }

      // if we were full before, and now we're not, post an event
      if (!was_writable && copy > 0) 
      {
         //PostEvent(owner_, SE_WRITE, 0);
         if(pFifo->pNotifyWriteCB)
            pFifo->pNotifyWriteCB(0);
      }
   }
   pthread_mutex_unlock(&pFifo->mutex);   
   
   return result;
}


///////////////////////////////////////////////////////////////////////////////
/// @par Declaration
///  Write into |buffer| with offset 0 from the current write position. <br>
///  This method doesn't adjust the number of buffered bytes, user has to call <br>
///  ConsumeWriteBuffer() to do this. <br>
///
/// @param pFifo
///   The pointer of tFIFO_BUFFER.<br>
///
/// @param buffer
///   The data to write.<br>
///
/// @param bytes
///   The size user want to write.<br>
///
/// @param bytes_read
///   The size of bytes wrote.<br>
///
/// @return
///   None:         return with tStreamResult.<br>
///
/// @par Conditions
///   None.<br>
///
/// @par Usage
///   None.<br>
///
///////////////////////////////////////////////////////////////////////////////
tStreamResult FIFO_Write(tFIFO_BUFFER *pFifo,
                  const void* buffer, 
                  size_t bytes,
                  size_t* bytes_written)
{
   //CritScope cs(&crit_);

   bool was_readable = false;
   size_t copy = 0;
   tStreamResult result = SR_SUCCESS;
   
   if(!pFifo) {
      return SR_ERROR;
   }
    
   pthread_mutex_lock(&pFifo->mutex);
#ifdef CHECK_COPY_BUFFER
    int i=0;
    // omit the control packet
    for (i = 3; i < bytes-3; ++i) {
        if(((char *)buffer)[i]==0x00) {
            ASSERT(((char *)buffer)[i]!=0x00);
        }
    }
#endif
    
   was_readable = (pFifo->data_length > 0);
   result = WriteOffsetLocked(pFifo, buffer, bytes, 0, &copy);

   if (result == SR_SUCCESS) {
#ifdef CHECK_COPY_BUFFER
       size_t vTmp = MIN(bytes, pFifo->buffer_length);
       if(copy!=vTmp) {
           ASSERT(copy==vTmp);
       }
#endif
      // If write was successful then adjust the number of readable bytes.
      pFifo->data_length += copy;
      if (bytes_written) 
      {
         *bytes_written = copy;
      }

      // if we didn't have any data to read before, and now we do, post an event
      if (!was_readable && copy > 0) 
      {
         //PostEvent(owner_, SE_READ, 0);
         if(pFifo->pNotifyReadCB)
            pFifo->pNotifyReadCB(0);
      }
   }
   else{
       ASSERT(result == SR_SUCCESS);
   }
   pthread_mutex_unlock(&pFifo->mutex);   
   
   return result;
}

void FIFO_Close(tFIFO_BUFFER *pFifo)
{
   //CritScope cs(&crit_);
   pthread_mutex_lock(&pFifo->mutex);
   if(pFifo)
   {
      pFifo->state = SS_CLOSED;
   }
   pthread_mutex_unlock(&pFifo->mutex);
}

const void* FIFO_GetReadData(tFIFO_BUFFER *pFifo, size_t* size)
{
   //CritScope cs(&crit_);

    
   U8 *vpPtr = NULL;
   if(!pFifo) {
       return NULL;
   }
    
   pthread_mutex_lock(&pFifo->mutex);
   *size = (pFifo->read_position + pFifo->data_length <= pFifo->buffer_length) ?
   pFifo->data_length : pFifo->buffer_length - pFifo->read_position;
   vpPtr = &pFifo->buffer[pFifo->read_position];
   pthread_mutex_unlock(&pFifo->mutex);
   
   return (void *)vpPtr;
}


void FIFO_ConsumeReadData(tFIFO_BUFFER *pFifo, size_t size)
{

    
   bool was_writable = false;
   
   //CritScope cs(&crit_);
   if(!pFifo) {
      return;
   }
      
   pthread_mutex_lock(&pFifo->mutex);
   //ASSERT(size <= data_length_);
   if(size > pFifo->data_length)
   {
      // TODO: error control
      pthread_mutex_unlock(&pFifo->mutex);
      return;
   }
   
   was_writable = pFifo->data_length < pFifo->buffer_length;
   pFifo->read_position = (pFifo->read_position + size) % pFifo->buffer_length;
   pFifo->data_length -= size;
   if (!was_writable && size > 0) 
   {
      //PostEvent(owner_, SE_WRITE, 0);
      if(pFifo->pNotifyWriteCB)
         pFifo->pNotifyWriteCB(0);
   }
   pthread_mutex_unlock(&pFifo->mutex);      
}


void* FIFO_GetWriteBuffer(tFIFO_BUFFER *pFifo, size_t* size)
{
   //CritScope cs(&crit_);

    
   if(!pFifo) {
     return NULL;
   }
    
   pthread_mutex_lock(&pFifo->mutex);
   if (pFifo->state == SS_CLOSED) {
      pthread_mutex_unlock(&pFifo->mutex);
      return NULL;
   }

   // if empty, reset the write position to the beginning, so we can get
   // the biggest possible block
   if (pFifo->data_length == 0) 
   {
      pFifo->read_position = 0;
   }

   U8 *vpPtr = NULL;
   const size_t write_position = (pFifo->read_position + pFifo->data_length)
   % pFifo->buffer_length;
    
   // The caculation of size may incorrect, but since we don't use this function
   // so fix it in the future
   *size = (write_position > pFifo->read_position || pFifo->data_length == 0) ?
   pFifo->buffer_length - write_position : pFifo->read_position - write_position;
    
//    *size = (write_position > pFifo->read_position || pFifo->data_length == 0) ?
//    pFifo->buffer_length - write_position + pFifo->read_position : pFifo->read_position - write_position;
//
    
   vpPtr = &pFifo->buffer[write_position];
   pthread_mutex_unlock(&pFifo->mutex);   
   
   return vpPtr;
}

void FIFO_ConsumeWriteBuffer(tFIFO_BUFFER *pFifo, size_t size)
{

   bool was_readable = false;
   //CritScope cs(&crit_);
   if(!pFifo) {
      return;
   }
    
   pthread_mutex_lock(&pFifo->mutex);
   ASSERT(size <= pFifo->buffer_length - pFifo->data_length);
   
   was_readable = (pFifo->data_length > 0);
   pFifo->data_length += size;
   if (!was_readable && size > 0) 
   {
      //PostEvent(owner_, SE_READ, 0);
      if(pFifo->pNotifyReadCB)
         pFifo->pNotifyReadCB(0);
   }
   pthread_mutex_unlock(&pFifo->mutex);      
}


bool FIFO_GetWriteRemaining(tFIFO_BUFFER *pFifo, size_t* size)
{
   //CritScope cs(&crit_);

    
    if(!pFifo) {
      return false;
    }
    
   pthread_mutex_lock(&pFifo->mutex);
   *size = pFifo->buffer_length - pFifo->data_length;
   pthread_mutex_unlock(&pFifo->mutex);
   
   return true;
}
    
    
tStreamResult ReadOffsetLocked(tFIFO_BUFFER *pFifo,
                               void* buffer,
                               size_t bytes,
                               size_t offset,
                               size_t* bytes_read)
{
    if(!pFifo)
        return SR_ERROR;
    
    if (offset >= pFifo->data_length) {
        return (pFifo->state != SS_CLOSED) ? SR_BLOCK : SR_EOS;
    }
    
    const size_t available = pFifo->data_length - offset;
    const size_t read_position = (pFifo->read_position + offset) % pFifo->buffer_length;
    const size_t copy = MIN(bytes, available);
    const size_t tail_copy = MIN(copy, pFifo->buffer_length - read_position);
    //const size_t tail_copy = MIN(copy, pFifo->data_length - read_position);
    
    //char* const p = static_cast<char*>(buffer);
    char* const p = (char *)buffer;
    memcpy(p, &pFifo->buffer[read_position], tail_copy);
    memcpy(p + tail_copy, &pFifo->buffer[0], copy - tail_copy);
    
    if (bytes_read) {
        *bytes_read = copy;
    }
    return SR_SUCCESS;
}

tStreamResult WriteOffsetLocked(tFIFO_BUFFER *pFifo,
                                const void* buffer,
                                size_t bytes,
                                size_t offset,
                                size_t* bytes_written)
{
    if(!pFifo)
        return SR_ERROR;
    
    if (pFifo->state == SS_CLOSED) {
        return SR_EOS;
    }
    
    if (pFifo->data_length + offset >= pFifo->buffer_length) {
        return SR_BLOCK;
    }
    
    const size_t available = pFifo->buffer_length - pFifo->data_length - offset;
    const size_t write_position = (pFifo->read_position + pFifo->data_length + offset)
    % pFifo->buffer_length;
    const size_t copy = MIN(bytes, available);
    const size_t tail_copy = MIN(copy, pFifo->buffer_length - write_position);
    //const char* const p = static_cast<const char*>(buffer);
    //char* const p = (const char *)buffer;
    char* const p = (char *)buffer;
    memcpy(&pFifo->buffer[write_position], p, tail_copy);
//    if(copy > tail_copy) {
//        printf("copy > tail_copy (%zu,%zu)\n", copy, tail_copy);
//    }
    memcpy(&pFifo->buffer[0], p + tail_copy, copy - tail_copy);
    
    if (bytes_written) {
        *bytes_written = copy;
    }
    return SR_SUCCESS;
}

    
/*****************************************************************************/

#ifdef __cplusplus
}
#endif

/**  @} */
/******************************************************************************
 *  CONFIDENTIAL
 *****************************************************************************/
