#ifndef INCLUDED_ATOMICINT_HXX
#define INCLUDED_ATOMICINT_HXX

#include <malloc.h> // _aligned_malloc()/_aligned_free()

#define NOMINMAX // avoid min/max macros from windows.h
#define NOGDI // avoid GDI stuff (messes up juce)
#include <windows.h> // InterlockedXyz()

#include <cassert>

class AtomicInt
{
public:
	AtomicInt()
	{
		v_ = NULL;
		v_ = (volatile LONG *)(_aligned_malloc(sizeof(LONG), 4));
	}

	~AtomicInt()
	{
		_aligned_free((void *)(v_));
		v_ = NULL;
	}

	void operator=(int v)
	{
		assert(v >= 0); // interlocked methods ignore sign bit
		::InterlockedExchange(v_, (LONG)v);
	}

	bool operator==(int v) const
	{
		return (*v_ == (LONG)v);
	}

	bool operator!=(int v) const
	{
		return !(*this == v);
	}

	operator int() const
	{
		return int(*v_);
	}


private:
	volatile LONG *v_;

	AtomicInt(const AtomicInt &); // non-copyable
	AtomicInt &operator=(const AtomicInt &); // non-copyable
};

#endif

