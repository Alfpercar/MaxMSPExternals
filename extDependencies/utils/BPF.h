#ifndef __BPF__
#define __BPF__

#include <vector>
#include <cfloat>

// break point function
//
// for x-values before the first x, the first y will be returned
// for x-values after the last x, the last y will be returned
// 
// when calculating the surface, this will be also applied
//
class BPF
{
	private:
		std::vector<double> xpoints;
		std::vector<double> ypoints;
		std::vector<double> slopes;
		int search;
		double _yMin, _yMax;
	public:
		BPF()
		:search(0)
		{
		}
		double yMin() { return _yMin; }
		double yMax() { return _yMax; }
		void add(double x,double y)
		{
			if (!xpoints.empty())
			{
				if (x < xpoints.back()) throw "BPF x values should be consecutive";
				if (x == xpoints.back())
				{
					slopes.push_back(0.);
				}else{
					slopes.push_back((y - ypoints.back()) / (x - xpoints.back()));
				}
				if (y < _yMin) _yMin = y;
				if (y > _yMax) _yMax = y;
			}
			else 
			{
				_yMin = y;
				_yMax = y;
			}
			xpoints.push_back(x);
			ypoints.push_back(y);
		}
		int size(void)
		{
			return xpoints.size();
		}
		double concreteSurface(double x0,double x1)
		{
			// for testing only
			double x;
			double s = 0;
			int i = 0;
			double dx = (x1 - x0)/10000.;
			for (x = x0; x < x1; x+= dx)
			{
				s += get(x);
				i++;
			}
			s /= double(i);
			s *= (x1 - x0);
			return s;
		}
		double surface(double x0,double x1)
		{
			// calculate the integral or surface of the BPF
			double y0;
			double y1;
			double s = 0;
			int i0 = 0;
			int i1 = 0;
			double x;

			x = x0;
			if (x >= xpoints.back())
			{
				y0 = ypoints.back();
				i0 = xpoints.size() - 1;
			}else{
				while (x + DBL_EPSILON >= xpoints[i0]) i0++;
				if (i0==0)
					y0 = ypoints[0];
				else
					y0 = ypoints[i0-1] + (x0 - xpoints[i0-1])*slopes[i0-1];
			}

			x = x1;
			if (x >= xpoints.back())
			{
				y1 = ypoints.back();
				i1 = xpoints.size()-1;
			}else{
				while (x + DBL_EPSILON >= xpoints[i1]) i1++;
				if (i1) i1--;
				if (i1==0)
					y1 = ypoints[0];
				else
					y1 = ypoints[i1] + (x1 - xpoints[i1])*slopes[i1];
			}
	
			s += (xpoints[i0] - x0)*(y0 + (ypoints[i0]-y0)/2.);
			s += (x1 - xpoints[i1])*(y1 + (ypoints[i1]-y1)/2.);
			//printf("  %f\n",s);
			//int n = xpoints.size();
			for (int i=i0;i<i1;i++)
			{
				s += (xpoints[i+1] - xpoints[i])*(ypoints[i] + (ypoints[i+1]-ypoints[i])/2.);
				//printf("  %f %f %f %f %f\n",xpoints[i],xpoints[i+1],ypoints[i],slopes[i],s);
			}
			//printf(" surface = %f\n",s);
			return s;
		}
		double get(double x)
		{
			if (x < xpoints[search]) search = 0;
			if (x >= xpoints.back()) return ypoints.back();
			while (x + DBL_EPSILON >= xpoints[search]) search++;
			if (search==0) return ypoints[0];
			search--;
		  return (x - xpoints[search]) * slopes[search] + ypoints[search];
		}
};

#endif
