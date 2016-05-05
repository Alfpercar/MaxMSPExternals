#ifndef INCLUDED_DERIVATIVE2_HXX
#define INCLUDED_DERIVATIVE2_HXX

// Esteban style 2 sample derivative:
// dx[i] = (x[i+1] - x[i-1])/(2/fs)
//
// But, without using future samples (for causality):
// dx2[i] = (x[i] - x[i-2])/(2/fs)
//
// So the centered derivative is equal to the causal derivative advanced by one sample:
// dx[i] = dx2[i+1)
class Derivative2
{
public:
	Derivative2()
	{
		xNmin1_ = 0.0f;
		xNmin2_ = 0.0f;
	}

	float compute(float xN, float fs)
	{
		float dxN = (xN - xNmin2_)/(2.0f/fs);

		xNmin2_ = xNmin1_;
		xNmin1_ = xN;

		return dxN;
	}

private:
	float xNmin1_;
	float xNmin2_;
};

#endif