#ifndef INCLUDED_FIXEDFREQTONE_HXX
#define INCLUDED_FIXEDFREQTONE_HXX

#include "concat/Utilities/FloatToInt.hxx"

#include <cmath>

// Generates a band-limited saw/triangle wave at a fixed frequency.
// Uses additive synthesis to fill a wave table (lut).
class FixedFreqTone
{
public:
	enum WaveformType
	{
        SAW,
		TRIANGLE
	};

	FixedFreqTone()
	{
		lut_ = NULL;
		offset_ = 0.0f;
	}

	~FixedFreqTone()
	{
		delete[] lut_;
		lut_ = NULL;
	}

	void generateTable(double f0, double fs, WaveformType waveformType)
	{
		const double fn = fs/2.0; // nyquist frequency
		const int K = concat::floor_int(fn/f0); // number of partials below nyquist

		const int N = 2048; // lut size

		delete[] lut_;
		lut_ = NULL;

		lut_ = new float[N];
		for (int n = 0; n < N; ++n)
		{
			lut_[n] = 0.f;
		}

		const double pi = 3.1415926535897932384626433832795;

		if (waveformType == SAW)
		{
			// fourier series:
			for (int k = 1; k <= K; ++k)
			{
				for (int n = 0; n < N; ++n)
				{
					double t = (double)(n)/(double)(N); // [0;1[
					double f = 1.0; // 1 cycle in lut

					lut_[n] += (float)( sin(2*pi*k*f*t)/(double)(k) );
				}
			}

			// scale and invert:
			for (int n = 0; n < N; ++n)
			{
				lut_[n] = (float)(-2/pi*lut_[n]);
			}
		}
		else if (waveformType == TRIANGLE)
		{
			// fourier series:
			for (int k = 1; k <= K; ++k)
			{
				for (int n = 0; n < N; ++n)
				{
					double t = (double)(n)/(double)(N); // [0;1[
					double f = 1.0; // 1 cycle in lut

					lut_[n] += (float)( sin((k*pi)/2) * sin(2*pi*k*f*t)/(k*k) );
				}
			}

			// scale:
			for (int n = 0; n < N; ++n)
			{
				lut_[n] = (float)(8/(pi*pi)*lut_[n]);
			}
		}

		lutSize_ = N;
		stride_ = (float)(f0*N/fs);
	}

	void synthesize2(float *out1, float *out2, int numSamples)
	{
		if (lut_ == NULL)
			return;

		for (int i = 0; i < numSamples; ++i)
		{
			int offsetLhs = concat::floor_int(offset_);
			int offsetRhs = ((offsetLhs + 1) % lutSize_);
			float delta = offset_ - offsetLhs;
			float out = (1.0f - delta)*lut_[offsetLhs] + delta*lut_[offsetRhs];
			offset_ = fmod(offset_ + stride_, (float)(lutSize_));

//			out1[i] += 0.25f*out;
//			out2[i] += 0.25f*out;
			out1[i] = 0.25f*out;
			out2[i] = 0.25f*out;
		}

		// XXX: could also use fixed-point for the lut indexing
		// if table size is N = 2^M, use M bits for integer part and (32 - M) bits for 
		// fractional part
		// allows modulus to be done automatically by inherent wrapping
	}

	void synthesize1(float *out1, int numSamples)
	{
		if (lut_ == NULL)
			return;

		for (int i = 0; i < numSamples; ++i)
		{
			int offsetLhs = concat::floor_int(offset_);
			int offsetRhs = ((offsetLhs + 1) % lutSize_);
			float delta = offset_ - offsetLhs;
			float out = (1.0f - delta)*lut_[offsetLhs] + delta*lut_[offsetRhs];
			offset_ = fmod(offset_ + stride_, (float)(lutSize_));

			out1[i] = 0.25f*out;
		}
	}

private:
	float *lut_;
	int lutSize_;
	float offset_;
	float stride_;
};

#endif