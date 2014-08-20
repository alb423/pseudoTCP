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

/*** PROJECT INCLUDES ********************************************************/
#include <CUnit/CUnit.h>
#include <CUnit/Console.h>
#include "mi_types.h"
#include "fifo_buffer.h"
/*** MACROS ******************************************************************/


/*** GLOBAL VARIABLE DECLARATIONS (EXTERN) ***********************************/


/*** PRIVATE TYPES DEFINITIONS ***********************************************/


/*** PRIVATE VARIABLE DECLARATIONS (STATIC) **********************************/


/*** PRIVATE FUNCTION PROTOTYPES *********************************************/


/*** PUBLIC FUNCTION DEFINITIONS *********************************************/

// Test FIFO
void NotifyRead(int i) { /*printf("NotifyRead\n")*/;}
void NotifyWrite(int i) { /*printf("NotifyWrite\n")*/;}

void FifoBufferTest_All(void)
{
  const size_t kSize = 16;
  const char in[33] = "0123456789ABCDEFGHIJKLMNOPQRSTUV";
  char out[32]; // kSize * 2
  void* p;
  const void* q;
  size_t bytes;
  
  tFIFO_BUFFER *pFifo;
  pFifo = FIFO_Create(kSize, NotifyRead, NotifyWrite);

  // Test assumptions about base state
  CU_ASSERT_EQUAL(SS_OPEN, FIFO_GetState(pFifo));
  CU_ASSERT_EQUAL(SR_BLOCK, FIFO_Read(pFifo, out, kSize, &bytes));
  CU_ASSERT_TRUE(NULL != FIFO_GetReadData(pFifo, &bytes));
  CU_ASSERT_EQUAL((size_t)0, bytes);
  FIFO_ConsumeReadData(pFifo, 0);
  CU_ASSERT_TRUE(NULL != FIFO_GetWriteBuffer(pFifo, &bytes));
  CU_ASSERT_EQUAL(kSize, bytes);
  FIFO_ConsumeWriteBuffer(pFifo, 0);

  // Try a full write
  CU_ASSERT_EQUAL(SR_SUCCESS, FIFO_Write(pFifo, in, kSize, &bytes));
  CU_ASSERT_EQUAL(kSize, bytes);

  // Try a write that should block
  CU_ASSERT_EQUAL(SR_BLOCK, FIFO_Write(pFifo, in, kSize, &bytes));

  // Try a full read
  CU_ASSERT_EQUAL(SR_SUCCESS, FIFO_Read(pFifo, out, kSize, &bytes));
  CU_ASSERT_EQUAL(kSize, bytes);
  CU_ASSERT_EQUAL(0, memcmp(in, out, kSize));

  // Try a read that should block
  CU_ASSERT_EQUAL(SR_BLOCK, FIFO_Read(pFifo, out, kSize, &bytes));

  // Try a too-big write
  CU_ASSERT_EQUAL(SR_SUCCESS, FIFO_Write(pFifo, in, kSize * 2, &bytes));
  CU_ASSERT_EQUAL(bytes, kSize);

  // Try a too-big read
  CU_ASSERT_EQUAL(SR_SUCCESS, FIFO_Read(pFifo, out, kSize * 2, &bytes));
  CU_ASSERT_EQUAL(kSize, bytes);
  CU_ASSERT_EQUAL(0, memcmp(in, out, kSize));

  // Try some small writes and reads
  CU_ASSERT_EQUAL(SR_SUCCESS, FIFO_Write(pFifo, in, kSize / 2, &bytes));
  CU_ASSERT_EQUAL(kSize / 2, bytes);
  CU_ASSERT_EQUAL(SR_SUCCESS, FIFO_Read(pFifo, out, kSize / 2, &bytes));
  CU_ASSERT_EQUAL(kSize / 2, bytes);
  CU_ASSERT_EQUAL(0, memcmp(in, out, kSize / 2));
  CU_ASSERT_EQUAL(SR_SUCCESS, FIFO_Write(pFifo, in, kSize / 2, &bytes));
  CU_ASSERT_EQUAL(kSize / 2, bytes);
  CU_ASSERT_EQUAL(SR_SUCCESS, FIFO_Write(pFifo, in, kSize / 2, &bytes));
  CU_ASSERT_EQUAL(kSize / 2, bytes);
  CU_ASSERT_EQUAL(SR_SUCCESS, FIFO_Read(pFifo, out, kSize / 2, &bytes));
  CU_ASSERT_EQUAL(kSize / 2, bytes);
  CU_ASSERT_EQUAL(0, memcmp(in, out, kSize / 2));
  CU_ASSERT_EQUAL(SR_SUCCESS, FIFO_Read(pFifo, out, kSize / 2, &bytes));
  CU_ASSERT_EQUAL(kSize / 2, bytes);
  CU_ASSERT_EQUAL(0, memcmp(in, out, kSize / 2));

  // Try wraparound reads and writes in the following pattern
  // WWWWWWWWWWWW.... 0123456789AB....
  // RRRRRRRRXXXX.... ........89AB....
  // WWWW....XXXXWWWW 4567....89AB0123
  // XXXX....RRRRXXXX 4567........0123
  // XXXXWWWWWWWWXXXX 4567012345670123
  // RRRRXXXXXXXXRRRR ....01234567....
  // ....RRRRRRRR.... ................
  CU_ASSERT_EQUAL(SR_SUCCESS, FIFO_Write(pFifo, in, kSize * 3 / 4, &bytes));
  CU_ASSERT_EQUAL(kSize * 3 / 4, bytes);
  CU_ASSERT_EQUAL(SR_SUCCESS, FIFO_Read(pFifo, out, kSize / 2, &bytes));
  CU_ASSERT_EQUAL(kSize / 2, bytes);
  CU_ASSERT_EQUAL(0, memcmp(in, out, kSize / 2));
  CU_ASSERT_EQUAL(SR_SUCCESS, FIFO_Write(pFifo, in, kSize / 2, &bytes));
  CU_ASSERT_EQUAL(kSize / 2, bytes);
  CU_ASSERT_EQUAL(SR_SUCCESS, FIFO_Read(pFifo, out, kSize / 4, &bytes));
  CU_ASSERT_EQUAL(kSize / 4 , bytes);
  CU_ASSERT_EQUAL(0, memcmp(in + kSize / 2, out, kSize / 4));
  CU_ASSERT_EQUAL(SR_SUCCESS, FIFO_Write(pFifo, in, kSize / 2, &bytes));
  CU_ASSERT_EQUAL(kSize / 2, bytes);
  CU_ASSERT_EQUAL(SR_SUCCESS, FIFO_Read(pFifo, out, kSize / 2, &bytes));
  CU_ASSERT_EQUAL(kSize / 2 , bytes);
  CU_ASSERT_EQUAL(0, memcmp(in, out, kSize / 2));
  CU_ASSERT_EQUAL(SR_SUCCESS, FIFO_Read(pFifo, out, kSize / 2, &bytes));
  CU_ASSERT_EQUAL(kSize / 2 , bytes);
  CU_ASSERT_EQUAL(0, memcmp(in, out, kSize / 2));

  // Use GetWriteBuffer to reset the read_position for the next tests
  FIFO_GetWriteBuffer(pFifo, &bytes);
  FIFO_ConsumeWriteBuffer(pFifo, 0);

  // Try using GetReadData to do a full read
  CU_ASSERT_EQUAL(SR_SUCCESS, FIFO_Write(pFifo, in, kSize, &bytes));
  q = FIFO_GetReadData(pFifo, &bytes);
  CU_ASSERT_TRUE(NULL != q);
  CU_ASSERT_EQUAL(kSize, bytes);
  CU_ASSERT_EQUAL(0, memcmp(q, in, kSize));
  FIFO_ConsumeReadData(pFifo, kSize);
  CU_ASSERT_EQUAL(SR_BLOCK, FIFO_Read(pFifo, out, kSize, &bytes));

  // Try using GetReadData to do some small reads
  CU_ASSERT_EQUAL(SR_SUCCESS, FIFO_Write(pFifo, in, kSize, &bytes));
  q = FIFO_GetReadData(pFifo, &bytes);
  CU_ASSERT_TRUE(NULL != q);
  CU_ASSERT_EQUAL(kSize, bytes);
  CU_ASSERT_EQUAL(0, memcmp(q, in, kSize / 2));
  FIFO_ConsumeReadData(pFifo, kSize / 2);
  q = FIFO_GetReadData(pFifo, &bytes);
  CU_ASSERT_TRUE(NULL != q);
  CU_ASSERT_EQUAL(kSize / 2, bytes);
  CU_ASSERT_EQUAL(0, memcmp(q, in + kSize / 2, kSize / 2));
  FIFO_ConsumeReadData(pFifo, kSize / 2);
  CU_ASSERT_EQUAL(SR_BLOCK, FIFO_Read(pFifo, out, kSize, &bytes));

  // Try using GetReadData in a wraparound case
  // WWWWWWWWWWWWWWWW 0123456789ABCDEF
  // RRRRRRRRRRRRXXXX ............CDEF
  // WWWWWWWW....XXXX 01234567....CDEF
  // ............RRRR 01234567........
  // RRRRRRRR........ ................
  CU_ASSERT_EQUAL(SR_SUCCESS, FIFO_Write(pFifo, in, kSize, &bytes));
  CU_ASSERT_EQUAL(SR_SUCCESS, FIFO_Read(pFifo, out, kSize * 3 / 4, &bytes));
  CU_ASSERT_EQUAL(SR_SUCCESS, FIFO_Write(pFifo, in, kSize / 2, &bytes));
  q = FIFO_GetReadData(pFifo, &bytes);
  CU_ASSERT_TRUE(NULL != q);
  CU_ASSERT_EQUAL(kSize / 4, bytes);
  CU_ASSERT_EQUAL(0, memcmp(q, in + kSize * 3 / 4, kSize / 4));
  FIFO_ConsumeReadData(pFifo, kSize / 4);
  q = FIFO_GetReadData(pFifo, &bytes);
  CU_ASSERT_TRUE(NULL != q);
  CU_ASSERT_EQUAL(kSize / 2, bytes);
  CU_ASSERT_EQUAL(0, memcmp(q, in, kSize / 2));
  FIFO_ConsumeReadData(pFifo, kSize / 2);

  // Use GetWriteBuffer to reset the read_position for the next tests
  FIFO_GetWriteBuffer(pFifo, &bytes);
  FIFO_ConsumeWriteBuffer(pFifo, 0);

  // Try using GetWriteBuffer to do a full write
  p = FIFO_GetWriteBuffer(pFifo, &bytes);
  CU_ASSERT_TRUE(NULL != p);
  CU_ASSERT_EQUAL(kSize, bytes);
  memcpy(p, in, kSize);
  FIFO_ConsumeWriteBuffer(pFifo, kSize);
  CU_ASSERT_EQUAL(SR_SUCCESS, FIFO_Read(pFifo, out, kSize, &bytes));
  CU_ASSERT_EQUAL(kSize, bytes);
  CU_ASSERT_EQUAL(0, memcmp(in, out, kSize));

  // Try using GetWriteBuffer to do some small writes
  p = FIFO_GetWriteBuffer(pFifo, &bytes);
  CU_ASSERT_TRUE(NULL != p);
  CU_ASSERT_EQUAL(kSize, bytes);
  memcpy(p, in, kSize / 2);
  FIFO_ConsumeWriteBuffer(pFifo, kSize / 2);
  p = FIFO_GetWriteBuffer(pFifo, &bytes);
  CU_ASSERT_TRUE(NULL != p);
  CU_ASSERT_EQUAL(kSize / 2, bytes);
  memcpy(p, in + kSize / 2, kSize / 2);
  FIFO_ConsumeWriteBuffer(pFifo, kSize / 2);
  CU_ASSERT_EQUAL(SR_SUCCESS, FIFO_Read(pFifo, out, kSize, &bytes));
  CU_ASSERT_EQUAL(kSize, bytes);
  CU_ASSERT_EQUAL(0, memcmp(in, out, kSize));

  // Try using GetWriteBuffer in a wraparound case
  // WWWWWWWWWWWW.... 0123456789AB....
  // RRRRRRRRXXXX.... ........89AB....
  // ........XXXXWWWW ........89AB0123
  // WWWW....XXXXXXXX 4567....89AB0123
  // RRRR....RRRRRRRR ................
  CU_ASSERT_EQUAL(SR_SUCCESS, FIFO_Write(pFifo, in, kSize * 3 / 4, &bytes));
  CU_ASSERT_EQUAL(SR_SUCCESS, FIFO_Read(pFifo, out, kSize / 2, &bytes));
  p = FIFO_GetWriteBuffer(pFifo, &bytes);
  CU_ASSERT_TRUE(NULL != p);
  CU_ASSERT_EQUAL(kSize / 4, bytes);
  memcpy(p, in, kSize / 4);
  FIFO_ConsumeWriteBuffer(pFifo, kSize / 4);
  p = FIFO_GetWriteBuffer(pFifo, &bytes);
  CU_ASSERT_TRUE(NULL != p);
  CU_ASSERT_EQUAL(kSize / 2, bytes);
  memcpy(p, in + kSize / 4, kSize / 4);
  FIFO_ConsumeWriteBuffer(pFifo, kSize / 4);
  CU_ASSERT_EQUAL(SR_SUCCESS, FIFO_Read(pFifo, out, kSize * 3 / 4, &bytes));
  CU_ASSERT_EQUAL(kSize * 3 / 4, bytes);
  CU_ASSERT_EQUAL(0, memcmp(in + kSize / 2, out, kSize / 4));
  CU_ASSERT_EQUAL(0, memcmp(in, out + kSize / 4, kSize / 4));

  // Check that the stream is now empty
  CU_ASSERT_EQUAL(SR_BLOCK, FIFO_Read(pFifo, out, kSize, &bytes));

  // Try growing the buffer
  CU_ASSERT_EQUAL(SR_SUCCESS, FIFO_Write(pFifo, in, kSize, &bytes));
  CU_ASSERT_EQUAL(kSize, bytes);
  CU_ASSERT_TRUE(FIFO_SetCapacity(pFifo, kSize * 2));
  CU_ASSERT_EQUAL(pFifo->buffer_length, kSize * 2);
  CU_ASSERT_EQUAL(SR_SUCCESS, FIFO_Write(pFifo, in + kSize, kSize, &bytes));
  CU_ASSERT_EQUAL(kSize, bytes);
  CU_ASSERT_EQUAL(SR_SUCCESS, FIFO_Read(pFifo, out, kSize * 2, &bytes));
  CU_ASSERT_EQUAL(kSize * 2, bytes);
  CU_ASSERT_EQUAL(0, memcmp(in, out, kSize * 2));

  // Try shrinking the buffer
  CU_ASSERT_EQUAL(SR_SUCCESS, FIFO_Write(pFifo, in, kSize, &bytes));
  CU_ASSERT_EQUAL(kSize, bytes);
  CU_ASSERT_TRUE(FIFO_SetCapacity(pFifo, kSize));
  CU_ASSERT_EQUAL(SR_BLOCK, FIFO_Write(pFifo, in, kSize, &bytes));
  CU_ASSERT_EQUAL(SR_SUCCESS, FIFO_Read(pFifo, out, kSize, &bytes));
  CU_ASSERT_EQUAL(kSize, bytes);
  CU_ASSERT_EQUAL(0, memcmp(in, out, kSize));

  // Write to the stream, close it, read the remaining bytes
  CU_ASSERT_EQUAL(SR_SUCCESS, FIFO_Write(pFifo, in, kSize / 2, &bytes));
  FIFO_Close(pFifo);
  CU_ASSERT_EQUAL(SS_CLOSED, FIFO_GetState(pFifo));
  CU_ASSERT_EQUAL(SR_EOS, FIFO_Write(pFifo, in, kSize / 2, &bytes));
  CU_ASSERT_EQUAL(SR_SUCCESS, FIFO_Read(pFifo, out, kSize / 2, &bytes));
  CU_ASSERT_EQUAL(0, memcmp(in, out, kSize / 2));
  CU_ASSERT_EQUAL(SR_EOS, FIFO_Read(pFifo, out, kSize / 2, &bytes));
}

void FifoBufferTest_FullBufferCheck(void)
{
   tFIFO_BUFFER *pFifo;
   pFifo = FIFO_Create(10, NotifyRead, NotifyWrite);

   FIFO_ConsumeWriteBuffer(pFifo, 10);
   
   size_t free;
   CU_ASSERT_TRUE(FIFO_GetWriteBuffer(pFifo, &free) != NULL);
   CU_ASSERT_EQUAL(0U, free);
}

void FifoBufferTest_WriteOffsetAndReadOffset(void) 
{
   const size_t kSize = 16;
   const char in[33] = "0123456789ABCDEFGHIJKLMNOPQRSTUV";
   char out[32];

   tFIFO_BUFFER *pFifo;
   pFifo = FIFO_Create(kSize, NotifyRead, NotifyWrite);
   
   // Write 14 bytes.
   CU_ASSERT_EQUAL(SR_SUCCESS, FIFO_Write(pFifo, in, 14, NULL));

   // Make sure data is in |buf|.
   size_t buffered;
   CU_ASSERT_TRUE(FIFO_GetBuffered(pFifo,&buffered));
   CU_ASSERT_EQUAL(14u, buffered);

   // Read 10 bytes.
   FIFO_ConsumeReadData(pFifo, 10);

   // There should be now 12 bytes of available space.
   size_t remaining;
   CU_ASSERT_TRUE(FIFO_GetWriteRemaining(pFifo, &remaining));
   CU_ASSERT_EQUAL(12u, remaining);

   // Write at offset 12, this should fail.
   CU_ASSERT_EQUAL(SR_BLOCK, FIFO_WriteOffset(pFifo, in, 10, 12, NULL));

   // Write 8 bytes at offset 4, this wraps around the buffer.
   CU_ASSERT_EQUAL(SR_SUCCESS, FIFO_WriteOffset(pFifo, in, 8, 4, NULL));

   // Number of available space remains the same until we call
   // ConsumeWriteBuffer().
   CU_ASSERT_TRUE(FIFO_GetWriteRemaining(pFifo, &remaining));
   CU_ASSERT_EQUAL(12u, remaining);
   FIFO_ConsumeWriteBuffer(pFifo, 12);

   // There's 4 bytes bypassed and 4 bytes no read so skip them and verify the
   // 8 bytes written.
   size_t read;
   CU_ASSERT_EQUAL(SR_SUCCESS, FIFO_ReadOffset(pFifo, out, 8, 8, &read));
   CU_ASSERT_EQUAL(8u, read);
   CU_ASSERT_EQUAL(0, memcmp(out, in, 8));

   // There should still be 16 bytes available for reading.
   CU_ASSERT_TRUE(FIFO_GetBuffered(pFifo, &buffered));
   CU_ASSERT_EQUAL(16u, buffered);

   // Read at offset 16, this should fail since we don't have that much data.
   CU_ASSERT_EQUAL(SR_BLOCK, FIFO_ReadOffset(pFifo, out, 10, 16, NULL));
}

/*** Test for List *********************************************/



/*** Test for PseudoTCP *********************************************/

/*****************************************************************************/

#ifdef __cplusplus
}
#endif

/**  @} */
/******************************************************************************
 *  CONFIDENTIAL
 *****************************************************************************/
