#ifndef INCLUDED_INTERPOLATION_HXX
#define INCLUDED_INTERPOLATION_HXX

#include <cmath>
#include <cassert>

#pragma warning(push)
#pragma warning(disable : 4244)

// Unwraps a wrapped sequence p(n) in radians (i.e. wrapped in a 2 pi range), iteratively.
// Based on Matlab's unwrap() algorithm.
//
// Say p(n) is wrapped in [-pi;+pi[ and at some point p(n) exceeds +pi and wraps around to 
// -pi at p(n-1).
// So the change at that point will be -2pi. Wrapping this change mod((-2pi)+pi, 2pi)-pi = 0.
// The correction of the derivative then becomes 0 - (-2pi) = +2pi. After integration 
// this correction becomes an offset of +2pi for all points of p(n) after the wrap.
class UnwrapIterative
{
public:
	UnwrapIterative()
	{
		pNmin1 = 0.0f;
		pCorrNmin1 = 0.0f;        
	}

	float unwrap(float pN)
	{
		const float pi = 3.1415926535897932384626433832795f;

		float pUnwrappedN; // result

		// Compute derivative of p(n) over n:
		float pDiffN = pN - pNmin1; // derivative so that pDiff(0) = p(0)

		// Wrap derivative in range [-pi;+pi[
		float pDiffWrappedN = fmod(pDiffN + pi, 2*pi) - pi;

		// Preserve +pi case at +pi, instead of wrapping to -pi:
		if (pDiffWrappedN == -pi && pDiffN > 0.0f)
			pDiffWrappedN = pi;

		// Compute correction of derivative:
		float pDiffCorrN = pDiffWrappedN - pDiffN;

		// Ignore corrections < cutoff (also to avoid for rounding issues):
		const float cutoff = pi; // tolerance
		if (pDiffCorrN < cutoff)
			pDiffCorrN = 0.0f;

		// Compute correction of p(n) by integrating correction of derivative:
		float pCorrN = pCorrNmin1 + pDiffCorrN; // cumulative sum

		// Compute unwrapped p(n):
		pUnwrappedN = pN + pCorrN;
		
		// Update state for next call:
		pNmin1 = pN;
		pCorrNmin1 = pCorrN;

		return pUnwrappedN;
	}

private:
	float pNmin1;
	float pCorrNmin1;
};

// ---------------------------------------------------------------------------------------

inline float wrap(float p)
{
	const float pi = 3.1415926535897932384626433832795f;
	return fmod(p + pi, 2*pi) - pi;
}

// ---------------------------------------------------------------------------------------

inline float interp1Linear(float xNmin1, float xN, float delta)
{
	assert(delta >= 0.0f && delta <= 1.0f);
	return xNmin1 + delta*(xN - xNmin1); // xNmin1*(1.0f - delta) + xN*delta
}

// ---------------------------------------------------------------------------------------

#include "ComputeDescriptors.hxx"

template<int NUM_SENSORS>
class TrackerFrameInterpolator
{
public:
	void interpolate(const RawSensorData &lhs, const RawSensorData &rhs, float delta, RawSensorData &interp)
	{
		assert(NUM_SENSORS == 3);

		// sensor 1 position:
		interp.violinBodySensPos(0, 0) = interp1Linear(lhs.violinBodySensPos(0, 0), rhs.violinBodySensPos(0, 0), delta);
		interp.violinBodySensPos(1, 0) = interp1Linear(lhs.violinBodySensPos(1, 0), rhs.violinBodySensPos(1, 0), delta);
		interp.violinBodySensPos(2, 0) = interp1Linear(lhs.violinBodySensPos(2, 0), rhs.violinBodySensPos(2, 0), delta);

		// sensor 1 orientation:
		interp.violinBodySensOrientation(0, 0) = wrap(interp1Linear(unwrapLhs_[0].unwrap(lhs.violinBodySensOrientation(0, 0)), unwrapRhs_[0].unwrap(rhs.violinBodySensOrientation(0, 0)), delta));
		interp.violinBodySensOrientation(1, 0) = wrap(interp1Linear(unwrapLhs_[1].unwrap(lhs.violinBodySensOrientation(1, 0)), unwrapRhs_[1].unwrap(rhs.violinBodySensOrientation(1, 0)), delta));
		interp.violinBodySensOrientation(2, 0) = wrap(interp1Linear(unwrapLhs_[2].unwrap(lhs.violinBodySensOrientation(2, 0)), unwrapRhs_[2].unwrap(rhs.violinBodySensOrientation(2, 0)), delta));

		// sensor 2 position:
		interp.bowSensPos(0, 0) = interp1Linear(lhs.bowSensPos(0, 0), rhs.bowSensPos(0, 0), delta);
		interp.bowSensPos(1, 0) = interp1Linear(lhs.bowSensPos(1, 0), rhs.bowSensPos(1, 0), delta);
		interp.bowSensPos(2, 0) = interp1Linear(lhs.bowSensPos(2, 0), rhs.bowSensPos(2, 0), delta);

		// sensor 2 orientation:
		interp.bowSensOrientation(0, 0) = wrap(interp1Linear(unwrapLhs_[0].unwrap(lhs.bowSensOrientation(0, 0)), unwrapRhs_[0].unwrap(rhs.bowSensOrientation(0, 0)), delta));
		interp.bowSensOrientation(1, 0) = wrap(interp1Linear(unwrapLhs_[1].unwrap(lhs.bowSensOrientation(1, 0)), unwrapRhs_[1].unwrap(rhs.bowSensOrientation(1, 0)), delta));
		interp.bowSensOrientation(2, 0) = wrap(interp1Linear(unwrapLhs_[2].unwrap(lhs.bowSensOrientation(2, 0)), unwrapRhs_[2].unwrap(rhs.bowSensOrientation(2, 0)), delta));

		// sensor 3 position:
		interp.stylusSensPos(0, 0) = interp1Linear(lhs.stylusSensPos(0, 0), rhs.stylusSensPos(0, 0), delta);
		interp.stylusSensPos(1, 0) = interp1Linear(lhs.stylusSensPos(1, 0), rhs.stylusSensPos(1, 0), delta);
		interp.stylusSensPos(2, 0) = interp1Linear(lhs.stylusSensPos(2, 0), rhs.stylusSensPos(2, 0), delta);

		// sensor 3 orientation:
		interp.stylusSensOrientation(0, 0) = wrap(interp1Linear(unwrapLhs_[0].unwrap(lhs.stylusSensOrientation(0, 0)), unwrapRhs_[0].unwrap(rhs.stylusSensOrientation(0, 0)), delta));
		interp.stylusSensOrientation(1, 0) = wrap(interp1Linear(unwrapLhs_[1].unwrap(lhs.stylusSensOrientation(1, 0)), unwrapRhs_[1].unwrap(rhs.stylusSensOrientation(1, 0)), delta));
		interp.stylusSensOrientation(2, 0) = wrap(interp1Linear(unwrapLhs_[2].unwrap(lhs.stylusSensOrientation(2, 0)), unwrapRhs_[2].unwrap(rhs.stylusSensOrientation(2, 0)), delta));

		// ext. sync flag:
		if (lhs.extSyncFlag && rhs.extSyncFlag)
			interp.extSyncFlag = true;
		else
			interp.extSyncFlag = false;

		// sensor 3 button pressed:
		if (lhs.stylusButtonPressed && rhs.stylusButtonPressed)
			interp.stylusButtonPressed = true;
		else
			interp.stylusButtonPressed = false;
	}

private:
	UnwrapIterative unwrapLhs_[3*NUM_SENSORS];
	UnwrapIterative unwrapRhs_[3*NUM_SENSORS];
};

#pragma warning(pop)

#endif

