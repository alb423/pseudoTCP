/**
 * @file    fifo_buffer.h
 * @brief   fifo_buffer include file
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

#ifndef __FIFO_BUFFER_H__
#define __FIFO_BUFFER_H__

#ifdef __cplusplus
extern "C" {
#endif

/*** STANDARD INCLUDES *******************************************************/
#include <pthread.h>

/*** PROJECT INCLUDES ********************************************************/
#include "mi_types.h"
#include "mi_util.h"
#include "pseudo_tcp_int.h"
/*** MACROS ******************************************************************/


/*** GLOBAL TYPES DEFINITIONS ************************************************/
typedef void  (* tFIFO_CB) (void *, int) ;

typedef struct tFIFO_BUFFER {
    tStreamState state;  // keeps the opened/closed state of the stream
    size_t buffer_length;  // size of the allocated buffer
    size_t data_length;  // amount of readable data in the buffer
    size_t read_position;  // offset to the readable data
    pthread_mutex_t mutex;
    tFIFO_CB  pNotifyReadCB;
    tFIFO_CB  pNotifyWriteCB;
    U8 *buffer;  // the allocated buffer
} tFIFO_BUFFER;


/*** PRIVATE TYPES DEFINITIONS ***********************************************/


/*** GLOBAL VARIABLE DECLARATIONS (EXTERN) ***********************************/


/*** PUBLIC FUNCTION PROTOTYPES **********************************************/

// Creates a FIFO buffer with the specified capacity.
tFIFO_BUFFER * FIFO_Create(size_t length, tFIFO_CB pNotifyReadCB, tFIFO_CB pNotifyWriteCB);

// Destroy a FIFO buffer.
void FIFO_Destroy(tFIFO_BUFFER *ptr);

// Gets the amount of data currently readable from the buffer.
bool FIFO_GetBuffered(tFIFO_BUFFER *ptr, size_t* size);
tStreamState FIFO_GetState(tFIFO_BUFFER *pFifo);

// Resizes the buffer to the specified capacity. Fails if data_length_ > size
bool FIFO_SetCapacity(tFIFO_BUFFER *pFifo, size_t length);

// Read into |buffer| with an offset from the current read position, offset
// is specified in number of bytes.
// This method doesn't adjust read position nor the number of available
// bytes, user has to call ConsumeReadData() to do this.
tStreamResult FIFO_ReadOffset(tFIFO_BUFFER *pFifo,
                              void* buffer,
                              size_t bytes,
                              size_t offset,
                              size_t* bytes_read);

// Write |buffer| with an offset from the current write position, offset is
// specified in number of bytes.
// This method doesn't adjust the number of buffered bytes, user has to call
// ConsumeWriteBuffer() to do this.
tStreamResult FIFO_WriteOffset(tFIFO_BUFFER *pFifo,
                               const void* buffer,
                               size_t bytes,
                               size_t offset,
                               size_t* bytes_written);

tStreamResult FIFO_Read(tFIFO_BUFFER *pFifo,
                        void* buffer,
                        size_t bytes,
                        size_t* bytes_read);

tStreamResult FIFO_Write(tFIFO_BUFFER *pFifo,
                         const void* buffer,
                         size_t bytes,
                         size_t* bytes_written);


void FIFO_Close(tFIFO_BUFFER *pFifo);
const void* FIFO_GetReadData(tFIFO_BUFFER *pFifo, size_t* data_len);
void FIFO_ConsumeReadData(tFIFO_BUFFER *pFifo, size_t used);
void* FIFO_GetWriteBuffer(tFIFO_BUFFER *pFifo, size_t* buf_len);
void FIFO_ConsumeWriteBuffer(tFIFO_BUFFER *pFifo, size_t used);
bool FIFO_GetWriteRemaining(tFIFO_BUFFER *pFifo, size_t* size);

/*****************************************************************************/

#ifdef __cplusplus
}
#endif

#endif /* End of #ifndef __FIFO_BUFFER_H__ */

/******************************************************************************
 *  CONFIDENTIAL
 *****************************************************************************/
