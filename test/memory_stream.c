/**
 * @file    memory_stream.c
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
#include <CUnit/CUnit.h>
#include <CUnit/Console.h>
#include "mi_types.h"
#include "mi_util.h"
#include "memory_stream.h"
/*** MACROS ******************************************************************/
#if __LINUX__
#define ALIGNP(p, t) ((U8 *)((((U32)(p) + ((t) - 1)) & ~((t) - 1))))
#elif __APPLE__
#define ALIGNP(p, t) ((U8 *)((((U64)(p) + ((t) - 1)) & ~((t) - 1))))
#endif

/*** GLOBAL VARIABLE DECLARATIONS (EXTERN) ***********************************/
static const int kAlignment = 16;

/*** PRIVATE TYPES DEFINITIONS ***********************************************/


/*** PRIVATE VARIABLE DECLARATIONS (STATIC) **********************************/


/*** PRIVATE FUNCTION PROTOTYPES *********************************************/


/*** PUBLIC FUNCTION DEFINITIONS *********************************************/

extern tMemoryStream * MS_Init(const void* pData, size_t length)
{
    tMemoryStream *pStream = NULL;
    pStream = malloc(sizeof(tMemoryStream));
    if(pStream) {
        pStream->buffer_length_ = length;
        pStream->data_length_ = 0;
        pStream->seek_position_ = 0;

        pStream->buffer_alloc_ = malloc(length + kAlignment);
        memset(pStream->buffer_alloc_, 0, length + kAlignment);
        pStream->buffer_ = (char *)(ALIGNP(pStream->buffer_alloc_, kAlignment));
        if(pData) {
            memcpy(pStream->buffer_, pData, length);
            pStream->data_length_ = length;
        }
        return pStream;
    } else
        return NULL;
}

extern void MS_Destroy(tMemoryStream *pStream)
{
    if(pStream) {
        if(pStream->buffer_alloc_)
            free(pStream->buffer_alloc_);
        free(pStream);
    }
}


tStreamResult MS_DoReserve(tMemoryStream *pStream, size_t size, int* error)
{
    return (pStream->buffer_length_ >= size) ? SR_SUCCESS : SR_EOS;
}


tStreamState MS_GetState(tMemoryStream *pStream)
{
    return SS_OPEN;
}


tStreamResult MS_Read(tMemoryStream *pStream,
                      void* buffer,
                      size_t bytes,
                      size_t* bytes_read,
                      int* error)
{
    if(!pStream)
        return SR_ERROR;//SS_CLOSED;

    if (pStream->seek_position_ >= pStream->data_length_) {
        return SR_EOS;
    }
    size_t available = pStream->data_length_ - pStream->seek_position_;
    if (bytes > available) {
        // Read partial buffer
        bytes = available;
    }
    memcpy(buffer, &pStream->buffer_[pStream->seek_position_], bytes);
    pStream->seek_position_ += bytes;
    if (bytes_read) {
        *bytes_read = bytes;
    }
    return SR_SUCCESS;
}


tStreamResult MS_Write(tMemoryStream *pStream,
                       const void* buffer,
                       size_t bytes,
                       size_t* bytes_written,
                       int* error)
{
    if(!pStream)
        return SR_ERROR;//SS_CLOSED;

    size_t available = pStream->buffer_length_ - pStream->seek_position_;
    if (0 == available) {
        // Increase buffer size to the larger of:
        // a) new position rounded up to next 256 bytes
        // b) double the previous length
        size_t new_buffer_length = MAX(((pStream->seek_position_ + bytes) | 0xFF) + 1,
                                       pStream->buffer_length_ * 2);
        tStreamResult result = MS_DoReserve(pStream, new_buffer_length, error);
        if (SR_SUCCESS != result) {
            return result;
        }
        //ASSERT(pStream->buffer_length_ >= new_buffer_length);
        available = pStream->buffer_length_ - pStream->seek_position_;
    }

    if (bytes > available) {
        bytes = available;
    }
    memcpy(&pStream->buffer_[pStream->seek_position_], buffer, bytes);
    pStream->seek_position_ += bytes;
    if (pStream->data_length_ < pStream->seek_position_) {
        pStream->data_length_ = pStream->seek_position_;
    }
    if (bytes_written) {
        *bytes_written = bytes;
    }
    return SR_SUCCESS;
}


void MS_Close(tMemoryStream *pStream)
{
    // nothing to do
}

bool MS_SetPosition(tMemoryStream *pStream,size_t position)
{
    if(!pStream)
        return false;

    if (position > pStream->data_length_)
        return false;

    pStream->seek_position_ = position;
    return true;
}


bool MS_GetPosition(tMemoryStream *pStream,size_t* position)
{
    if(!pStream)
        return false;

    if (position)
        *position = pStream->seek_position_;
    return true;
}


bool MS_GetSize(tMemoryStream *pStream,size_t* size)
{
    if(!pStream)
        return false;

    if (size)
        *size = pStream->data_length_;
    return true;
}


bool MS_GetAvailable(tMemoryStream *pStream,size_t* size)
{
    if(!pStream)
        return false;

    if (size)
        *size = pStream->data_length_ - pStream->seek_position_;
    return true;
}


bool MS_ReserveSize(tMemoryStream *pStream, size_t size)
{
    return (SR_SUCCESS == MS_DoReserve(pStream, size, NULL));
}

char* MS_GetBuffer(tMemoryStream *pStream)
{
    if(pStream)
        return pStream->buffer_;
    else
        return NULL;
}


bool MS_Rewind(tMemoryStream *pStream)
{
    return MS_SetPosition(pStream, 0);
}

/*****************************************************************************/

#ifdef __cplusplus
}
#endif

/**  @} */
/******************************************************************************
 *  CONFIDENTIAL
 *****************************************************************************/
