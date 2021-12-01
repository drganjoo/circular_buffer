#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <assert.h>

typedef unsigned int SAMPLE;

typedef struct CBUFFER_H
{
	unsigned size;		/* Size of the circular buffer in SAMPLE units (NOT bytes) */
	SAMPLE* base_addr;	/* Base address of the circular buffer */
	SAMPLE* write_ptr;	/* Write pointer */
	SAMPLE* read_ptr;	/* Read pointer */
} CBUFFER_T;

/* Function to create a circular buffer and return a handle to it
 * size: Number of SAMPLE units that the buffer can hold.
 */
CBUFFER_T* cBufferCreate(unsigned size)
{
	// one extra byte has to be kept as when the queue is full we cannot
	// write to write_ptr and move it forward as write_ptr will == read_ptr
	// and that would mean empty buffer
	size++;

	CBUFFER_T* buffer = malloc(sizeof(CBUFFER_T) + sizeof(SAMPLE) * size);
	if (buffer)
	{
		buffer->size = size;
		buffer->base_addr = (SAMPLE*)((unsigned char*)buffer + sizeof(CBUFFER_T));
		buffer->write_ptr = buffer->read_ptr = buffer->base_addr;
	}

	return buffer;
}

/* Destroys a cbuffer that has been created previously
 * Returns:  0  - Success
 *           !0 - Failed
 */
int cBufferDestroy(CBUFFER_T* cbuffer)
{
	if (cbuffer)
	{
		free(cbuffer);
		// cbuffer = NULL;
		return 0;
	}

	return -1;
}

/* Returns the amount of space (in SAMPLE units) of the circular buffer.
 * The amount of space just after creation should be the buffer size (empty buffer)
 */
unsigned cBufferCalcAmountSpace(CBUFFER_T* cbuffer)
{
	unsigned int units = (cbuffer->read_ptr - cbuffer->write_ptr);
	if (cbuffer->read_ptr <= cbuffer->write_ptr)
		units += cbuffer->size;

	return units - 1;
}

/* Returns the amount of data (in SAMPLE units) ready to be read from the circular buffer.
 * The amount of data just after creation is 0 (empty buffer)
 */
unsigned cBufferCalcAmountData(CBUFFER_T* cbuffer)
{
	unsigned int units = (cbuffer->write_ptr - cbuffer->read_ptr);
	if (cbuffer->write_ptr < cbuffer->read_ptr)
		units += cbuffer->size;

	return units;
}

/* Reads n SAMPLE units from cbuffer and stores them into a linear buffer linearBuff if there are
 * enough samples available. If there are not enough samples available to be read, then nothing
 * gets read from cbuffer and returns failure.
 * Returns:  0 - Success
 *          !0 - Failed
 */
int cBufferRead(CBUFFER_T* cbuffer, unsigned n, SAMPLE* linearBuff)
{
	if (cBufferCalcAmountData(cbuffer) < n)
		return -1;

	SAMPLE* end = cbuffer->base_addr + cbuffer->size;
	while (n--)
	{
		*linearBuff = *cbuffer->read_ptr;
		linearBuff++;
		cbuffer->read_ptr++;

		if (cbuffer->read_ptr >= end)
			cbuffer->read_ptr = cbuffer->base_addr;
	}

	return 0;
}

/* Writes n SAMPLE units from a linear buffer (linearBuff) to the cbuffer if there's enough
 * space available. If there's not enough space available to write all the n samples then nothing
 * gets written and returns failure.
 * Returns:  0 - Success
 *          !0 - Failed
 */
int cBufferWrite(CBUFFER_T* cbuffer, unsigned n, const SAMPLE* linearBuff)
{
	if (cBufferCalcAmountSpace(cbuffer) < n)
		return -1;

	SAMPLE* end = cbuffer->base_addr + cbuffer->size;
	while (n--)
	{
		*cbuffer->write_ptr = *linearBuff;
		cbuffer->write_ptr++;
		linearBuff++;

		if (cbuffer->write_ptr >= end)
			cbuffer->write_ptr = cbuffer->base_addr;
	}

	return 0;
}

#ifndef RunTests
int main(void)
{
	printf("size of CBUFFER_H structure is: %d bytes\n", sizeof(struct CBUFFER_H));
	CBUFFER_T *t = cBufferCreate(10);
	assert(cBufferCalcAmountData(t) == 0);
	assert(cBufferCalcAmountSpace(t) == 10);

	SAMPLE linear[] = { 1,2,3,4,5,6,7,8,9,10 };
	cBufferWrite(t, 10, linear);

	assert(cBufferCalcAmountSpace(t) == 0);
	assert(cBufferCalcAmountData(t) == 10);

	memset(linear, 0, sizeof(linear));

	cBufferRead(t, 6, linear);
	assert(4 == cBufferCalcAmountData(t));
	assert(6 == cBufferCalcAmountSpace(t));
	
	int result = cBufferWrite(t, 8, linear);
	assert(-1 == result);

	linear[0] = 100;
	linear[1] = 200;
	linear[2] = 300;

	cBufferWrite(t, 3, linear);
	assert(3 == cBufferCalcAmountSpace(t));
	assert(7 == cBufferCalcAmountData(t));

	memset(linear, 0, sizeof(linear));

	// cannot read more data than in the buffer
	result = cBufferRead(t, 9, linear);
	assert(-1 == result);

	linear[0] = 900;
	cBufferWrite(t, 1, linear);
	assert(8 == cBufferCalcAmountData(t));
	assert(2 == cBufferCalcAmountSpace(t));

	memset(linear, 0, sizeof(linear));

	cBufferRead(t, 8, linear);
	assert(linear[0] == 7);
	assert(linear[1] == 8);
	assert(linear[2] == 9);
	assert(linear[3] == 10);
	assert(linear[4] == 100);
	assert(linear[5] == 200);
	assert(linear[6] == 300);
	assert(linear[7] == 900);

	assert(10 == cBufferCalcAmountSpace(t));
	assert(0 ==  cBufferCalcAmountData(t));

	cBufferDestroy(t);
	assert(-1 == cBufferDestroy(t));

	return 0;
}
#endif