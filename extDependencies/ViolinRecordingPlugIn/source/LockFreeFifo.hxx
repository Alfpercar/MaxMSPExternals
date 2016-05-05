#ifndef INCLUDED_LOCKFREEFIFO_HXX
#define INCLUDED_LOCKFREEFIFO_HXX

#include <cstddef>
#include <vector>
#include <cassert>
#include <malloc.h> // _aligned_malloc()/_aligned_free()

#define NOMINMAX // avoid min/max macros from windows.h
#define NOGDI // avoid GDI stuff (messes up juce)
#include <windows.h> // InterlockedXyz()

#include <algorithm> // min()/max()

// Single reader, single writer, lock-free FIFO class (implemented as a circular buffer, using Win32 API), 
// adapted from example by gasm.
//
// Lock-free mechanism:
// Writes are done using interlocked (atomic, memory barrier) methods (provided by the OS).
// Reads of properly aligned memory are assumed to be atomic.
// Because this does not guarantee that multiple successive reads will give the same result, 
// the algorithm is designed so that multiple reads of variables are only done from the threads 
// that uniquely writes that variable.
// E.g. The put() methods only read the read index once, but read the write index multiple times. 
// But the put() methods are the only functions that set the write index, so assuming there's only 
// a single thread calling these put() methods, the multiple reads of the write index will always 
// give the same value. The same is done for the get() methods, but the other way around.
//
// Note on FIFO capacity:
// Because read and write indexes are circular, the case of read_idx == write_idx has to be either defined 
// 'full' or 'empty', but not both (in our case it is defined as 'empty'). To be able to differentiate between 'empty' 
// and 'full' cases, the 'full' case has to be defined as write_idx == wrap(read_idx - 1). Thus the last element can't 
// be written, so the buffer can only hold capacity - 1 elements.
//
// Behavior when full/empty:
// When trying to write when the buffer is full, no data will be written (until data is read).
// When trying to read when the buffer is empty, no data will be read (until data is written).
//
// Limitations:
// The source/destination of writes/reads can only be other memory, not e.g. a file on disk.
template<typename Ty>
class LockFreeFifo
{
public:
	LockFreeFifo();
	~LockFreeFifo();

	void reserve(int capacity); // actual capacity will be capacity - 1 (see above)
	int getCapacity() const; // returns actual capacity

	void clearBySettingToZero();
	void clearBySettingReadIdxToWriteIdx();

	__declspec(noinline) int put(const Ty &datum, bool always = false);
	__declspec(noinline) int get(Ty &datum);
	__declspec(noinline) int put(const Ty *data, LONG size);
	__declspec(noinline) int get(Ty *data, LONG size);
	// Note: Turn off inlining to avoid compiler optimizations (typically not done across 
	// call boundaries) as much as possible.

	// get read/write indexes, e.g. useful for index based events
	LONG getWriteIdx() const;
	LONG getReadIdx() const;

	// dist = wrap(rhsIdx - lhsIdx) (where rhsIdx == lhsIdx case returns 0)
	// useful e.g. when computing the number of elements between the current read index 
	// and some index (of say an event) ahead of it
	LONG wrappedDistance(LONG lhsIdx, LONG rhsIdx);

	LONG getReadAvail() const; // result will be [0;size[
	LONG getWriteAvail() const; // result will be [0;size[

	void decreaseReadIdx(LONG n);

private:
	volatile LONG *writeIdx_;
	volatile LONG *readIdx_;
	// Note: 'volatile LONG' rather than 'unsigned int' or something because of Win32 
	// interlocked API (doesn't consider sign bit and must be volatile).

	std::vector<Ty> buffer_;
	LONG size_;

	LockFreeFifo(const LockFreeFifo &); // non-copyable
	LockFreeFifo &operator=(const LockFreeFifo &); // non-copyable
};

// ---------------------------------------------------------------------------------------

template<typename Ty>
LockFreeFifo<Ty>::LockFreeFifo()
{
	writeIdx_ = NULL;
	readIdx_ = NULL;

	writeIdx_ = (LONG *)(_aligned_malloc(sizeof(LONG), 4));
	readIdx_ = (LONG *)(_aligned_malloc(sizeof(LONG), 4));
	// NOTE: 
	// Rather than using LONG members directly for read/write indexes, use aligned memory allocation.
	// According to the C++ object allocation model, members are aligned with respect to object offset.
	// To ensure reads of these indexes are atomic (as in done as a whole, uninterrupted), they need to be 
	// aligned to 4 bytes on multi-cpu systems.

	// Initialize to empty:
	clearBySettingToZero();
	size_ = 0;
}

template<typename Ty>
LockFreeFifo<Ty>::~LockFreeFifo()
{
	reserve(0);

	_aligned_free((void *)(writeIdx_));
	_aligned_free((void *)(readIdx_));

	writeIdx_ = NULL;
	readIdx_ = NULL;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

template<typename Ty>
void LockFreeFifo<Ty>::reserve(int capacity)
{
	assert(capacity >= 0);
	if (capacity < 0)
		capacity = 0;
	
	clearBySettingToZero();
	buffer_.clear();

	buffer_.reserve(capacity);
	if (capacity > 0)
		buffer_.insert(buffer_.end(), capacity, Ty());
	
	size_ = (LONG)buffer_.size();
}

template<typename Ty>
int LockFreeFifo<Ty>::getCapacity() const
{
	int actualCapacity = std::max((int)size_ - 1, 0);
	return actualCapacity;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

template<typename Ty>
void LockFreeFifo<Ty>::clearBySettingToZero()
{
	(*readIdx_) = 0;
	(*writeIdx_) = 0;
}

template<typename Ty>
void LockFreeFifo<Ty>::clearBySettingReadIdxToWriteIdx()
{
	// Atomically set read index to write index:
	::InterlockedExchange(readIdx_, (*writeIdx_));
}

// ---------------------------------------------------------------------------------------

// if always is true, always writes, even if full
// in this case getReadAvail()/getWriteAvail() may not give correct results
template<typename Ty>
int LockFreeFifo<Ty>::put(const Ty &datum, bool always)
{
	LONG nextWriteIdx;

	// Compute new index (with wrapping):
	if (((*writeIdx_) + 1) >= size_)
		nextWriteIdx = 0;
	else
		nextWriteIdx = (*writeIdx_) + 1;

	if (nextWriteIdx == (*readIdx_) && !always)
		return 0; // full

	// Put value at current write index:
	buffer_[(*writeIdx_)] = datum;

	// Atomically update write index:
	::InterlockedExchange(writeIdx_, nextWriteIdx);

	return 1;
}

// If empty, returns 0 (datum will not be valid).
template<typename Ty>
int LockFreeFifo<Ty>::get(Ty &datum)
{
	// Check if empty:
	if ((*readIdx_) == (*writeIdx_))
		return 0; // empty

	// Get value at current read index:
	datum = buffer_[(*readIdx_)];

	// Atomically update read index (with wrapping):
	if (((*readIdx_) + 1) >= size_)
		::InterlockedExchange(readIdx_, 0);
	else
		::InterlockedIncrement(readIdx_);

	return 1;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

template<typename Ty>
int LockFreeFifo<Ty>::put(const Ty *data, LONG size)
{
	assert(size >= 0/* && size <= size_ - 1*/);
	assert((*writeIdx_) >= 0 && (*writeIdx_) < size_);

	if (data == NULL || size <= 0 || size_ == 0/* || size > size_*/)
		return 0;

	// Compute wrapped distance between write index and read index:
	const LONG maxSize = getWriteAvail();

	// Compute actual read size min(requested_size, possible_size):
	size = std::min(size, maxSize);

	// Compute next write index (after write), with wrapping:
	LONG nextWriteIdx = *writeIdx_ + size;
	if (nextWriteIdx >= size_)
		nextWriteIdx -= size_;
	assert(nextWriteIdx >= 0 && nextWriteIdx < size_);

	// Copy data:
	if (nextWriteIdx >= *writeIdx_)
	{
		// Able to do one continuous copy:
		std::copy(data, data + size, &buffer_[0] + *writeIdx_);
	}
	else
	{
		// Do wrapped, discontinuous copy in two parts:
		const LONG n1 = size_ - *writeIdx_;
		std::copy(data, data + n1, &buffer_[0] + *writeIdx_);
		std::copy(data + n1, data + size, &buffer_[0]);
	}

	// Atomically update write index:
	::InterlockedExchange(writeIdx_, nextWriteIdx);

	return size;
}

template<typename Ty>
int LockFreeFifo<Ty>::get(Ty *data, LONG size)
{
	assert(size >= 0/* && size <= size_ - 1*/);
	assert((*readIdx_) >= 0 && (*readIdx_) < size_);

	if (data == NULL || size <= 0 || size_ == 0/* || size > size_*/)
		return 0;

	// Compute wrapped distance between read index and write index:
	const LONG maxSize = getReadAvail();

	// Compute actual read size min(requested_size, possible_size):
	size = std::min(size, maxSize);

	// Compute next read index (after read), with wrapping:
	LONG nextReadIdx = (*readIdx_) + size;
	if (nextReadIdx >= size_)
		nextReadIdx -= size_;
	assert(nextReadIdx >= 0 && nextReadIdx < size_);

	// Copy data:
	if (nextReadIdx >= *readIdx_)
	{
		// Able to do one continuous copy:
		std::copy(&buffer_[0] + *readIdx_, &buffer_[0] + *readIdx_ + size, data);
	}
	else
	{
		// Do wrapped, discontinuous copy in two parts:
		const LONG n1 = size_ - *readIdx_;
		std::copy(&buffer_[0] + *readIdx_, &buffer_[0] + *readIdx_ + n1, data);
		std::copy(&buffer_[0], &buffer_[0] + size - n1, data + n1);
	}

	// Atomically update read index:
	::InterlockedExchange(readIdx_, nextReadIdx);

	return size;
}

// ---------------------------------------------------------------------------------------

template<typename Ty>
LONG LockFreeFifo<Ty>::getWriteIdx() const
{
	return *writeIdx_;
}

template<typename Ty>
LONG LockFreeFifo<Ty>::getReadIdx() const
{
	return *readIdx_;
}

// ---------------------------------------------------------------------------------------

template<typename Ty>
LONG LockFreeFifo<Ty>::getReadAvail() const
{
	if (size_ == 0)
		return 0;

	// Copy write index to temporary:
	// NOTE: Because we need to use writeIdx twice, we don't want it 
	// to change between uses. As it will only advance (or stay constant), 
	// using an out-of-date copy will only cause the maximum possible 
	// read size to be limited slightly (which normally shouldn't be problem).
	volatile const LONG tmpWriteIdx = *writeIdx_;

	// >= so that if writeIdx == readIdx, readAvail = 0 (state = empty)
	if (tmpWriteIdx >= *readIdx_)
		return tmpWriteIdx - *readIdx_;
	else
		return size_ - *readIdx_ + tmpWriteIdx;

	// NOTE: If using write index directly (instead of copy), in case when writeIdx == readIdx - 1 
	// at the compare (it goes into the 2nd branch), and writeIdx advances in another thread in between, 
	// the result may be size (while result should be in the range [0;size[). If it goes into the 1st 
	// case and the writeIdx advances in another thread in between the statements, the rsult will be correct 
	// (just more up-to-date).
	// XXX: Perhaps writeIdx can't advance in between the case where writeIdx == readIdx - 1, because 
	// buffer is full in that case (so it's not possible to write anything more).
}

template<typename Ty>
LONG LockFreeFifo<Ty>::getWriteAvail() const
{
	if (size_ == 0)
		return 0;

	// Copy read index to temporary:
	// NOTE: Because we need to use readIdx twice, we don't want it 
	// to change between uses. As it will only advance (or stay constant), 
	// using an out-of-date copy will only cause the maximum possible 
	// write size to be limited slightly (which normally shouldn't be problem).
	volatile const LONG tmpReadIdx = *readIdx_;

	// > so that if readIdx == writeIdx, writeAvail = size-1 (state = empty)
	if (tmpReadIdx > *writeIdx_)
		return tmpReadIdx - *writeIdx_ - 1;
	else
		return size_ - *writeIdx_ + tmpReadIdx - 1;

	// NOTE: If using read index directly (instead of copy), in case when readIdx == writeIdx 
	// at the compare (it goes into the 2nd branch), and readIdx advances in another thread in between, 
	// the result may be size (while result should be in the range [0;size[). If it goes into the 1st 
	// case and readIdx advances in between the statements, the result will be correct (just more up-to-date).
	// XXX: Perhaps readIdx can't advance in between in the case where readIdx == writeIdx, because 
	// the buffer is empty in that case (so there's nothing to read).

	// NOTE: Minus one because can write all elements except last one (see notes above).
	// E.g. if writeIdx_ = 10 and readIdx_ = 20, the maximum value of the 
	// writeIdx_ after the operation is computed is readIdx_ - 1 (19), so the 
	// max. write size is 9, not 10.
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

template<typename Ty>
LONG LockFreeFifo<Ty>::wrappedDistance(LONG lhsIdx, LONG rhsIdx)
{
	// >= so that if rhsIdx == lhsIdx, dist = 0
	if (rhsIdx >= lhsIdx)
		return rhsIdx - lhsIdx;
	else
		return size_ - lhsIdx + rhsIdx;
}

// ---------------------------------------------------------------------------------------

template<typename Ty>
void LockFreeFifo<Ty>::decreaseReadIdx(LONG n)
{
	assert(n >= 0);
	if (n <= 0)
		return; // allow decreaseReadIdx(0) without allocating (avoid going into assert below)

	assert(size_ > 0);
	if (size_ == 0)
		return; // avoid getting stuck in infinite loop

	LONG newReadIdx = *readIdx_;
	newReadIdx -= n;

	while (newReadIdx < 0)
		newReadIdx += size_;

	::InterlockedExchange(readIdx_, newReadIdx);
}

#endif

