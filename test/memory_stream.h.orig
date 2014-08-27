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

#ifndef __MEMORY_STREAM_H__
#define __MEMORY_STREAM_H__

#ifdef __cplusplus
extern "C" {
#endif

/*** STANDARD INCLUDES *******************************************************/


/*** PROJECT INCLUDES ********************************************************/
#include "pseudo_tcp_int.h"

/*** MACROS ******************************************************************/


/*** GLOBAL TYPES DEFINITIONS ************************************************/


/*** PRIVATE TYPES DEFINITIONS ***********************************************/


/*** GLOBAL VARIABLE DECLARATIONS (EXTERN) ***********************************/
typedef struct tMemoryStream
{
  char* buffer_;
  char* buffer_alloc_;
  size_t buffer_length_;
  size_t data_length_;
  size_t seek_position_;
} tMemoryStream;


extern tMemoryStream * MS_Init(const void* data, size_t length);
extern void MS_Destroy(tMemoryStream *pStream);

extern tStreamState MS_GetState(tMemoryStream *pStream);
extern tStreamResult MS_Read(tMemoryStream *pStream, 
                     void* buffer, 
                     size_t bytes, 
                     size_t* bytes_read, 
                     int* error);
extern tStreamResult MS_Write(tMemoryStream *pStream,
                     const void* buffer, 
                     size_t bytes,
                     size_t* bytes_written, 
                     int* error);
extern void MS_Close(tMemoryStream *pStream);
extern bool MS_SetPosition(tMemoryStream *pStream, size_t position);
extern bool MS_GetPosition(tMemoryStream *pStream, size_t* position);
extern bool MS_GetSize(tMemoryStream *pStream, size_t* size);
extern bool MS_GetAvailable(tMemoryStream *pStream, size_t* size);
extern bool MS_ReserveSize(tMemoryStream *pStream, size_t size);
extern char* MS_GetBuffer(tMemoryStream *pStream);
extern bool MS_Rewind(tMemoryStream *pStream);


/*** PUBLIC FUNCTION PROTOTYPES **********************************************/


/** Doubly Linked List ******************************************************/


/*****************************************************************************/

#ifdef __cplusplus
}
#endif

#endif /* End of #ifndef __SEGMENT_LIST_UNITTEST_H__ */

/******************************************************************************
 *  CONFIDENTIAL
 *****************************************************************************/
