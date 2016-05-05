#ifndef INCLUDED_ATOMICPTR_HXX
#define INCLUDED_ATOMICPTR_HXX

#include <malloc.h> // _aligned_malloc()/_aligned_free()

#define NOMINMAX // avoid min/max macros from windows.h
#define NOGDI // avoid GDI stuff (messes up juce)
#include <windows.h> // InterlockedXyz()

#include <cassert>

template<typename T>
class AtomicPtr
{
public:
	AtomicPtr()
	{
		p_ = NULL;
		p_ = (volatile void **)(_aligned_malloc(sizeof(void *), 4));
	}

	~AtomicPtr()
	{
		_aligned_free((void *)(p_));
		p_ = NULL;
	}

	void operator=(T *p)
	{
		InterlockedExchangePointer(p_, (void *)p); // (macro)
	}

	operator T*()
	{
		return (T *)(*p_);
	}

	operator const T*() const
	{
		return (const T *)(*p_);
	}

	T *operator->()
	{
		return (T *)(*p_);
	}

private:
	volatile void **p_;

	AtomicPtr(const AtomicPtr &); // non-copyable
	AtomicPtr &operator=(const AtomicPtr &); // non-copyable
};

#endif

