#ifndef INCLUDED_ATOMICENUM_HXX
#define INCLUDED_ATOMICENUM_HXX

#include <malloc.h> // _aligned_malloc()/_aligned_free()

#define NOMINMAX // avoid min/max macros from windows.h
#define NOGDI // avoid GDI stuff (messes up juce)
#include <windows.h> // InterlockedXyz()

#include <cassert>

template<typename EnumT>
class AtomicEnum
{
public:
	AtomicEnum()
	{
		v_ = NULL;
		v_ = (volatile LONG *)(_aligned_malloc(sizeof(LONG), 4));
	}

	~AtomicEnum()
	{
		_aligned_free((void *)(v_));
		v_ = NULL;
	}

	void operator=(EnumT v)
	{
		assert(v >= 0); // interlocked methods ignore sign bit
		::InterlockedExchange(v_, (LONG)v);
	}

	void operator=(int v)
	{
		assert(v >= 0); // interlocked methods ignore sign bit
		::InterlockedExchange(v_, (LONG)v);
	}

	bool operator==(EnumT v) const
	{
		return (*v_ == (LONG)v);
	}

	bool operator!=(EnumT v) const
	{
		return !(*this == v);
	}

	bool operator==(int v) const
	{
		return (*v_ == (LONG)v);
	}

	bool operator!=(int v) const
	{
		return !(*this == v);
	}

	operator EnumT() const
	{
		return EnumT(*v_);
	}

private:
	volatile LONG *v_;

	AtomicEnum(const AtomicEnum &); // non-copyable
	AtomicEnum &operator=(const AtomicEnum &); // non-copyable
};

#endif

