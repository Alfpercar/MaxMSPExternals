#ifndef INCLUDED_CONCAT_ITERATORHELPERS_HXX
#define INCLUDED_CONCAT_ITERATORHELPERS_HXX

namespace concat
{
	template<typename IterT>
	inline IterT next(IterT &it)
	{
		IterT result = it;
        ++result;
		return result;
	}

	template<typename IterT>
	inline IterT prev(IterT &it)
	{
		IterT result = it;
        --result;
		return result;
	}
}

#endif

