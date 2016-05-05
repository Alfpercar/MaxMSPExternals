#ifndef INCLUDED_CONCAT_MATHHELPERS_HXX
#define INCLUDED_CONCAT_MATHHELPERS_HXX

#include <cmath>

namespace concat
{
	inline float log2(float x)
	{
		static float divln2 = 1.0f/log(2.0f); // (might not compile in C compilers)
		return log(x)*divln2;
	}

	// (not optimized, just convenience function)
	inline double expFactorToLinFactor(int factorPowerOfTwo)
	{
		if (factorPowerOfTwo >= 0)
			return (double)(1 << factorPowerOfTwo);
		else
			return 1.0/(double)(1 << -factorPowerOfTwo);
	}
}

#endif