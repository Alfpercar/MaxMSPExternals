#ifndef INCLUDED_WINDOWGEN_HXX
#define INCLUDED_WINDOWGEN_HXX

#include <cmath>

inline void computeGaussWindow(float *w, int n, float alpha = 2.5f)
{
	if (n < 0)
		return;

	if (alpha < 2.0f)
		alpha = 2.0f;

	int i = 0;
	for (int k = -(n-1)/2; k <= (n-1)/2; ++k)
	{
		assert(i >= 0 && i < n);
		float tmp = (alpha * (float)(k)/((float)(n)/2.f));
		w[i] = exp( (-1.f/2.f) * tmp*tmp );
		++i;
	}
}

#endif