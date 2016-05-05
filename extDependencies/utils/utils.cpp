#include <string>
#include "math.h"
#include "utils.h"

using namespace std;


float *computeDerivative(float *values, int nValues, int inframe)
{
	float *derivative=new float[nValues];
	// compute first derivative
	int minFrame, maxFrame;
	if (inframe>=0)
	{	minFrame=inframe;
		maxFrame=inframe+1;
	}
	else
	{
		minFrame=0;
		maxFrame=nValues;
	}
	for(int frameIdx=minFrame;frameIdx<maxFrame;frameIdx++)
	{
		int k;
		float meanr = 0.f;
		// different frame rate --> 44100/256.=172.265625 vs 240Hz
		int l = 2; // at 240Hz
		int r = 7; // at 240Hz
		int aux;
		//float trackerFR = 240.f; // tracker frame rate
		//float synthFR = 44100.f / 256.f; // synthesis frame rate
		//l = floorf(l * synthFR / trackerFR + .5f);
		//r = floorf(r * synthFR / trackerFR + .5f);
		for(k=l; k<=r; k++)
		{	aux=min(k,nValues);
			meanr += values[ frameIdx + aux];
		}
		float meanl = 0.f;
		for(k=-l; k>=-r; k--)
		{	aux=max(k,0);
			meanl += values[ frameIdx + aux];
		}
		meanr /= float(r-l+1.f);
		meanl /= float(r-l+1.f);
		derivative[frameIdx]= meanr - meanl;
	}

	return derivative;
}

float *computeSecondDerivative(float *values, int nValues, int inframe)
{// compute second derivative
	float *derivative=new float[nValues];
	int d=3;
	int minFrame, maxFrame;
	if (inframe>=0)
	{	minFrame=inframe;
		maxFrame=inframe+1;
	}
	else
	{
		minFrame=0;
		maxFrame=nValues;
	}
	for(int frameIdx=minFrame;frameIdx<maxFrame;frameIdx++)
	{ 
		//int d = 4; // at 240 Hz
		//float trackerFR = 240.f; // tracker frame rate
		//float synthFR = 44100.f / 256.f; // synthesis frame rate
		//d = (int) floorf(d * synthFR / trackerFR + .5f);
		
		int b=frameIdx-d;
		int e=frameIdx+d;
		if(b<0) b=0;
		if(e>=nValues)e=nValues-1;
		derivative[frameIdx] = values[e] - values[b];
	}
	return derivative;
}

