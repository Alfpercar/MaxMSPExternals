#ifndef INCLUDED_ATOMICFLAG_HXX
#define INCLUDED_ATOMICFLAG_HXX

#include <malloc.h> // _aligned_malloc()/_aligned_free()

#define NOMINMAX // avoid min/max macros from windows.h
#define NOGDI // avoid GDI stuff (messes up juce)
#include <windows.h> // InterlockedXyz()

class AtomicFlag
{
public:
	AtomicFlag()
	{
		flag_ = NULL;
		flag_ = (volatile LONG *)(_aligned_malloc(sizeof(LONG), 4));
	}

	~AtomicFlag()
	{
		_aligned_free((void *)(flag_));
		flag_ = NULL;
	}

	bool isSet() const
	{
		return (*flag_ != 0);
	}

	void set(bool flag)
	{
		if (flag)
			::InterlockedExchange(flag_, 1);
		else
			::InterlockedExchange(flag_, 0);
	}

private:
	volatile LONG *flag_;

	AtomicFlag(const AtomicFlag &); // non-copyable
	AtomicFlag &operator=(const AtomicFlag &); // non-copyable
};

#endif

