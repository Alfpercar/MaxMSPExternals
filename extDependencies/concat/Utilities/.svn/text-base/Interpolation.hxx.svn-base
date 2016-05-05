#ifndef INCLUDED_CONCAT_INTERPOLATION_HXX
#define INCLUDED_CONCAT_INTERPOLATION_HXX

#include <cassert>

namespace concat
{
	// Hermite cubic spline interpolation, with tangents at p0 and p1 fixed to be 
	// zero. Interpolates between p0 at t=0 and p1 at t=1 (t=[0;1]).
	inline float splineInterpolate(float p0, float p1, float t)
	{
		assert(t >= 0.0f && t <= 1.0f);

		// p := (p0, p1, m0, m1, t) -> (2*t^3 - 3*t^2 + 1)*p0 + (t^3 - 2*t^2 + t)*m0 + (-2*t^3 + 3*t^2)*p1 + (t^3 - t^2)*m1;

		const float tt = t*t;
		const float ttt = tt*t;

		return (2.f*ttt - 3.f*tt + 1.f)*p0 + (-2.f*ttt + 3.f*tt)*p1;
	}

	inline float linearInterpolate(float p0, float p1, float t)
	{
		assert(t >= 0.0f && t <= 1.0f);
		return p0 + (p1 - p0)*t;
	}

	inline float linearInterpolate(float p[2], float t)
	{
		assert(t >= 0.0f && t <= 1.0f);
		return p[0] + (p[1] - p[0])*t;
	}
}

#endif