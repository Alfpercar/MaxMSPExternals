#ifndef INCLUDED_CONCAT_POLYNOMIAL_HXX
#define INCLUDED_CONCAT_POLYNOMIAL_HXX

#include <cstddef>
#include <cassert>

namespace concat
{
	// Evaluates a polynomial using Horner's rule.
	//
	// Given k-th order polynomial:
	// y = \sum_{n=0}^{k} p(k-n)*x^n
	// = p(k) + p(k-1)*x + p(k-2)*x^2 + .. p(0)*x^k											(1)
	//
	// Evaluation using Horner's rule:
	// y = p(k) + (p(k-1) + (p(k-2) + .. (p(0)*x)*x)..)*x									(2)
	//
	// T should be type float or double
	// x is the point to compute
	// p is the polynomial coefficients array in ascending powers of x (size should be k+1)
	// k is the polynomial order (one less than the number of polynomial coefficients, as a 
	// 0-th order polynomial has one coefficient)
	//
	// there's a small backwards difference rounding error (there are ways to compensate for 
	// this error, but they come at a performance cost)
	//
	// n multiplications and n additions
	//
	// Note order of coefficients p is identical to Matlab's polyval() and polyfit().
	template<typename T>
	inline const T polyval(const T x, const T *p, const int k)
	{
		assert(k >= 0);
		assert(p != NULL);

		T result = p[0];
		for (int n = 1; n <= k; ++n)
		{
			result = result*x + p[n];
		}

		return result;
	}
}

#endif