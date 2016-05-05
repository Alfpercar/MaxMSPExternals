#ifndef __UTILS__
#define __UTILS__

#define MIN(a,b) ((a)<(b)?(a):(b))
#define MAX(a,b) ((a)<(b)?(b):(a))

float *computeDerivative(float *values, int nValues, int inframe);
float *computeSecondDerivative(float *values, int nValues, int inframe);

#endif 