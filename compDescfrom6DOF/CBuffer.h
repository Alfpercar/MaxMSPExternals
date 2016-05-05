#pragma once

#include <stdio.h>
#include <malloc.h>
#include "LibertyTracker.hxx"

class CBuffer
{
private:

    int         size;   /* maximum number of elements           */
    int         start;  /* index of oldest element              */
    int			count;
    LibertyTracker::ItemData   *elems;  /* vector of elements                   */
	
public:
	CBuffer(int size) 
	{
		this->size  = size;
		start = 0;
		count = 0;
		elems = (LibertyTracker::ItemData *)calloc(size, sizeof(LibertyTracker::ItemData));
	}
 
	void cbFree() {
		free(elems); /* OK if null */start=0; count=0; }
 
	void resetIdxs() {
		start=0; count=0; }

	int cbIsFull() {
		return count == size; }
	 
	int cbIsEmpty() {
		return count == 0; }

	void cbWrite(LibertyTracker::ItemData *elem) {
		int end = (start + count) % size;
		elems[end] = *elem;
		if (count == size)
			start = (start + 1) % size; /* full, overwrite */
		else
			++ count;
	}
	 
	void cbRead(LibertyTracker::ItemData *elem) {
		*elem = elems[start];
		start = (start + 1) % size;
		-- count;
	}
	
	int getSize()
	{ return size;}

	int getRIdx()
	{ return start;}

	int getCount()
	{ return count;}

	LibertyTracker::ItemData* cbGetBuffer()
	{
		return elems;
	}
	~CBuffer(void){cbFree();}
};



  
//int main(int argc, char **argv) {
//    CircularBuffer cb;
//    ElemType elem = {0};
// 
//    int testBufferSize = 10; /* arbitrary size */
//    cbInit(&cb, testBufferSize);
// 
//    /* Fill buffer with test elements 3 times */
//    for (elem.value = 0; elem.value < 3 * testBufferSize; ++ elem.value)
//        cbWrite(&cb, &elem);
// 
//    /* Remove and print all elements */
//    while (!cbIsEmpty(&cb)) {
//        cbRead(&cb, &elem);
//        printf("%d\n", elem.value);
//    }
// 
//    cbFree(&cb);
//    return 0;
//}