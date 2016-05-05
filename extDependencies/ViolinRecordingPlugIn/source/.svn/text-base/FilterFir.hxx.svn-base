#ifndef INCLUDED_FILTERFIR_HXX
#define INCLUDED_FILTERFIR_HXX

#include <cstddef>
#include <cassert>

// Apply a FIR filter b_i, with i=0..N-1, to input signal x(n), with
// n=0..M-1.
// Parameter inputStride should normally be 1. When set to 2 for instance 
// every other input is skipped (useful for downsampling filter for
// instance).
//
// A causal FIR filter is defined as, 
// y(n) = \sum_{i=0}^{N-1} b_i x(n-i),                                (1)
// which is equivalent to convolution of the input signal with the filter 
// coefficients.
//
// From equation (1) it can be shown that to compute one output sample N-1
// past input samples (plus the current input sample) are needed. The
// unknown input samples x(n) with n < 0 are usually assumed to be zero.
//
// This function is equivalent to Matlab's built-in conv() function, but
// truncates the filters output to the same length as the input. To obtain
// the entire output append length(b)-1 zeros to the inputs end before
// calling filter_fir(). This function is not optimized for Matlab but
// closer to a possible real-time C implementation of a FIR filter (i.e. 
// with buffering).
class FilterFir
{
public:
	FilterFir();
	~FilterFir();

	void init(const float *b, unsigned int length); // b is copied
	void deinit();

	void process(const float *x, float *y, unsigned int n, unsigned int inputStride = 1, unsigned int outputOffset = 0, unsigned int outputStride = 1);

private:
	float *b_;
	unsigned int length_;

	float *circBuffer_;
	unsigned int writePos_;
	unsigned int readPos_;
};


// ---------------------------------------------------------------------------------------

inline FilterFir::FilterFir()
{
	b_ = NULL;
	circBuffer_ = NULL;
	length_ = 0;
}

inline FilterFir::~FilterFir()
{
	deinit();
}

inline void FilterFir::init(const float *b, unsigned int length)
{
//		assert(length > 0);
//		if (length <= 0)
//			return;

	// Make sure filter length is even (pad coefficients with zero if not) to 
	// allow parallelization of inner loop pipelining (see process):
//		length_ = dsp::nextEvenNumber(length);
	length_ = length;

	// Pre-zeropad input (in circular buffer) with zeros:
	// This is effectively equivalent to delaying the input signal with
	// length(b)-1 samples. From now on write_pos is equivalent to n in equation
	// (1) above and read_pos is equivalent to n-(N-1).
	circBuffer_ = new float[length_];
	for (unsigned int i = 0; i < length_ - 1; ++i)
	{
		circBuffer_[i] = 0.f;
	}
	readPos_ = 0;
	writePos_ = length_ - 1;

	// Pre-time-reverse filter coefficients to avoid having to subtract indexes
	// in inner loop, as y(n) = \sum_{i=0}^{N-1} b_i x(n-i), is equivalent to
	// y(n) = \sum_{i=0}^{N-1} b_{(N-1)-i} x(n-(N-1)+i). Thus if b'_i is the
	// time-reversed version of b_i and x'(n) is the buffered version of x(n) 
	// delayed by N-1 samples, the computation becomes 
	// y(n) = \sum_{i=0}^{N-1} b'_i x'(n+i).
	b_ = new float[length_];

	int j;
	j = length_ - length; // 0 (length is even) or 1 (length is odd)
	if (j == 1)
		b_[0] = 0.f; // pad filter coefficient with zero if odd

	for (int i = length - 1; i >= 0; --i)
	{
		assert(j >= 0 && j < (int)length_);
		assert(i >= 0 && i < (int)length);
		b_[j] = b[i];
		++j;
	}
}

inline void FilterFir::deinit()
{
	delete[] b_;
	b_ = NULL;
	delete[] circBuffer_;
	circBuffer_ = NULL;
	length_ = 0;
}
	
inline void FilterFir::process(const float *x, float *y, unsigned int n, unsigned int inputStride, unsigned int outputOffset, unsigned int outputStride)
{
	// NOTE:
	// Buffering can be done in several ways:
	// 1. Wrapping read/write indexes.
	// 2. Data copies (shifting data through buffer).
	// 3. Using two flat (non-wrapping) inner loops instead of one wrapping inner loop.
	// 4. Using two flat (non-wrapping) outer loops repeatedly as well as two flat inner loops.
	// 5. Using double size circular buffers so wrapping isn't needed.
	// 6. Increasing circular buffer size to nearest 2^N so wrapping can be done using bit-masking.

	// Sample-by-sample (outer) loop:
	unsigned int k = outputOffset;
	for (unsigned int i = 0; i < n; ++i)
	{
		// Write input to end of circular buffer:
		circBuffer_[writePos_] = x[i];

		// Increment write position with wrapping:
		++writePos_;
		if (writePos_ >= length_)
			writePos_ = 0;
//			writePos_ = ((++writePos_) % length_);
		
		if ((i % inputStride) == 0)
		{
			// Compute convolution for y(i) (inner loop):
			// Avoid having to do circular buffer index wrapping in inner loop by 
			// splitting up the loop in two flat loops.
			float y_n = 0.f;

//				float acc1;
//				float acc2;

			const unsigned int n1 = length_ - readPos_;
//				const unsigned int n1_remaining = (n1 & 1);//(n1 % 2);
//				const unsigned int n1_even = n1 - n1_remaining;
//				acc1 = 0.f;
//				acc2 = 0.f;
//				for (unsigned int j = 0; j < n1_even; j += 2)
			for (unsigned int j = 0; j < n1; ++j)
			{
				// XXX: could do index offsets only once (copying pointers and offsetting them), 
				// but compiler will probably optimize this already
//					acc1 += b_[j]*circBuffer_[readPos_ + j];
//					acc2 += b_[j + 1]*circBuffer_[readPos_ + j + 1];
				y_n += b_[j]*circBuffer_[readPos_ + j];
			}
//				y_n += acc1 + acc2;
			// XXX: could do branch only once, because if n1_remaining is even then n2_remaining will also be even 
			// (because length_ is even)
//				if (n1_remaining)
//				{
//					y_n += b_[n1 - 1]*circBuffer_[readPos_ + n1 - 1];
//				}

			const unsigned int n2 = readPos_; // = length_ - n1
//				const unsigned int n2_remaining = (n2 & 1);//(n2 % 2);
//				const unsigned int n2_even = n2 - n2_remaining;
//				acc1 = 0.f;
//				acc2 = 0.f;
//				for (unsigned int j = 0; j < n2_even; j += 2)
			for (unsigned int j = 0; j < n2; ++j)
			{
//					acc1 += b_[n1 + j]*circBuffer_[j];
//					acc2 += b_[n1 + j + 1]*circBuffer_[j + 1];
				y_n += b_[n1 + j]*circBuffer_[j];
			}
//				y_n += acc1 + acc2;
//				if (n2_remaining)
//				{
//					y_n += b_[n1 - 1]*circBuffer_[readPos_ + n1 - 1];
//				}

			//
//				y_n += n2_remaining*(b_[n1 - 1]*circBuffer_[readPos_ + n1 - 1] + b_[n1 + n2 - 1]*circBuffer_[n1 + n2 - 1]);
			//

			// Increment (outer) read position with wrapping:
			readPos_ += inputStride; // ++readPos_ 
			if (readPos_ >= length_)
				readPos_ = readPos_ - length_; //readPos_ = 0;
			// XXX: will readPos_ - length_ always result in 0?  <<-- LOOKS LIKE IT!!, but probably not, eg with length = 22, stride=8
//				readPos_ = ((readPos_ + inputStride) % length_);

			// Copy to output array:
			y[k] = y_n;
			k += outputStride;
		}
	}
}

#endif