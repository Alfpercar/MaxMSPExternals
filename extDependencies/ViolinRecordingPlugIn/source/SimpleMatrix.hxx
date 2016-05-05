#ifndef INCLUDED_SIMPLEMATRIX_HXX
#define INCLUDED_SIMPLEMATRIX_HXX

#include <cassert>
#include <cmath>

// ---------------------------------------------------------------------------------------
// Contains some very simple (and incomplete) classes for doing matrix calculations with 
// 3x1, 3x3 and 4x4 matrices.
// ---------------------------------------------------------------------------------------

class Matrix3x1
{
public:
	Matrix3x1()
	{
	}

	Matrix3x1
		(
		double v00,
		double v10,
		double v20
		)
	{
		v_[0][0] = v00;
		v_[1][0] = v10;
		v_[2][0] = v20;
	}

	// -----------------------------------------------------------------------------------

	double operator()(int col, int row) const
	{
		// (unchecked)
		return v_[col][row];
	}

	double &operator()(int col, int row)
	{
		// (unchecked)
		return v_[col][row];
	}

	// -----------------------------------------------------------------------------------

	Matrix3x1 &operator*=(double rhs)
	{
		v_[0][0] *= rhs;
		v_[1][0] *= rhs;
		v_[2][0] *= rhs;

		return *this;
	}

	// (no other *= operators as they are ambiguous)

	// -----------------------------------------------------------------------------------

	const double *getPtr() const
	{
        return (double *)v_;
	}

private:
	double v_[3][1];
};

// ---------------------------------------------------------------------------------------

// 3x1 + 3x1:
inline Matrix3x1 operator+(const Matrix3x1 &lhs, const Matrix3x1 &rhs)
{
	return Matrix3x1(
		lhs(0, 0) + rhs(0,0),
		lhs(1, 0) + rhs(1,0),
		lhs(2, 0) + rhs(2,0));
}

// 3x1 - 3x1:
inline Matrix3x1 operator-(const Matrix3x1 &lhs, const Matrix3x1 &rhs)
{
	return Matrix3x1(
		lhs(0, 0) - rhs(0,0),
		lhs(1, 0) - rhs(1,0),
		lhs(2, 0) - rhs(2,0));
}

// 3x1 .* 1x1:
inline Matrix3x1 times(const Matrix3x1 &lhs, double rhs)
{
	return Matrix3x1(
		lhs(0, 0)*rhs,
		lhs(1, 0)*rhs,
		lhs(2, 0)*rhs);
}

inline Matrix3x1 operator*(const Matrix3x1 &lhs, double rhs)
{
	return times(lhs, rhs);
}

// 1x1 .* 3x1:
inline Matrix3x1 times(double lhs, const Matrix3x1 &rhs)
{
	return Matrix3x1(
		lhs*rhs(0, 0),
		lhs*rhs(1, 0),
		lhs*rhs(2, 0));
}

inline Matrix3x1 operator*(double lhs, const Matrix3x1 &rhs)
{
	return times(lhs, rhs);
}

// 3x1 .* 3x1:
// note: 3x1 * 3x1 is not possible
inline Matrix3x1 times(const Matrix3x1 &lhs, const Matrix3x1 &rhs)
{
	return Matrix3x1(
		lhs(0, 0)*rhs(0, 0),
		lhs(1, 0)*rhs(1, 0),
		lhs(2, 0)*rhs(2, 0));
}

//inline Matrix3x1 operator*(const Matrix3x1 &lhs, const Matrix3x1 &rhs)
//{
//	return times(lhs, rhs);
//}

// 3x1 ./ 1x1:
inline Matrix3x1 operator/(const Matrix3x1 &lhs, double rhs)
{
	return times(lhs, 1.0/rhs);
}

// 3x1 . 3x1:
inline double dot(const Matrix3x1 &lhs, const Matrix3x1 &rhs)
{
	return lhs(0, 0)*rhs(0, 0) + lhs(1, 0)*rhs(1, 0) + lhs(2, 0)*rhs(2, 0); // = sum(times(lhs, rhs))
}

// 3x1 x 3x1:
inline Matrix3x1 cross(const Matrix3x1 &lhs, const Matrix3x1 &rhs)
{
	return Matrix3x1(
		lhs(1, 0)*rhs(2, 0) - lhs(2, 0)*rhs(1, 0),
		lhs(2, 0)*rhs(0, 0) - lhs(0, 0)*rhs(2, 0),
		lhs(0, 0)*rhs(1, 0) - lhs(1, 0)*rhs(0, 0));
}

// sum(3x1):
inline double sum(const Matrix3x1 &v)
{
	return v(0, 0) + v(1, 0) + v(2, 0);
}

// 3x1^2:
inline Matrix3x1 square(const Matrix3x1 &v)
{
	return times(v, v);
}

// ||3x1||:
inline double euclidean_length(const Matrix3x1 &v)
{
	return sqrt(sum(square(v)));
}

// 3x1 ./ ||3x1||:
inline Matrix3x1 normalize(const Matrix3x1 &v)
{
	const double m = euclidean_length(v);
	if (m == 0.0)
		return v;

	return times(v, 1.0/m);
}

// ---------------------------------------------------------------------------------------
// ---------------------------------------------------------------------------------------
// ---------------------------------------------------------------------------------------

class Matrix3x3; // forward declaration
Matrix3x3 operator*(const Matrix3x3 &lhs, const Matrix3x3 &rhs); // forward declaration

class Matrix3x3
{
public:
	Matrix3x3()
	{
	}

	Matrix3x3
		(
		double v00, double v01, double v02,
		double v10, double v11, double v12,
		double v20, double v21, double v22
		)
	{
		v_[0][0] = v00; v_[0][1] = v01; v_[0][2] = v02;
		v_[1][0] = v10; v_[1][1] = v11; v_[1][2] = v12;
		v_[2][0] = v20; v_[2][1] = v21; v_[2][2] = v22;
	}

	// -----------------------------------------------------------------------------------

	double operator()(int col, int row) const
	{
		// (unchecked)
		return v_[col][row];
	}

	double &operator()(int col, int row)
	{
		// (unchecked)
		return v_[col][row];
	}

	// -----------------------------------------------------------------------------------

	Matrix3x3 &operator*=(double rhs)
	{
		v_[0][0] *= rhs; v_[0][1] *= rhs; v_[0][2] *= rhs;
		v_[1][0] *= rhs; v_[1][1] *= rhs; v_[1][2] *= rhs;
		v_[2][0] *= rhs; v_[2][1] *= rhs; v_[2][2] *= rhs;
		
		return *this;
	}

	// (no other *= operators as they are ambiguous)

	// -----------------------------------------------------------------------------------

	static Matrix3x3 identity()
	{
		return Matrix3x3(1.0, 0.0, 0.0, 
						 0.0, 1.0, 0.0, 
						 0.0, 0.0, 1.0);
	}

	static Matrix3x3 rotation_matrix_x(double angleRadians)
	{
		const double sr = sin(angleRadians);
		const double cr = cos(angleRadians);

		return Matrix3x3(
			1.0, 0.0, 0.0,
			0.0,  cr, -sr,
			0.0,  sr,  cr);
	}

	static Matrix3x3 rotation_matrix_y(double angleRadians)
	{
		const double sr = sin(angleRadians);
		const double cr = cos(angleRadians);

		return Matrix3x3(
			 cr, 0.0,  sr,
			0.0, 1.0, 0.0,
			-sr, 0.0,  cr);
	}

	static Matrix3x3 rotation_matrix_z(double angleRadians)
	{
		const double sr = sin(angleRadians);
		const double cr = cos(angleRadians);
		
		return Matrix3x3(
			 cr, -sr, 0.0,
			 sr,  cr, 0.0,
			0.0, 0.0, 1.0);
	}

	static Matrix3x3 rotation_matrix_zyx(double azimuth, double elevation, double roll)
	{
		//const double a = cos(roll);
		//const double b = sin(roll);
		//const double c = cos(elevation);
		//const double d = sin(elevation);
		//const double e = cos(azimuth);
		//const double f = sin(azimuth);

		//Matrix3x3 result(
		//	c*e,			-c*f,			d,
		//	b*d*e + a*f,	-b*d*f + a*e,	-b*c,
		//	-a*d*e + b*f,	a*d*f + b*e,	a*c);

		//return result;
		// XXX: optimized algorithm doesn't seem to give the same result

		Matrix3x3 rot_z = rotation_matrix_z(azimuth);
		Matrix3x3 rot_y = rotation_matrix_y(elevation);
		Matrix3x3 rot_x = rotation_matrix_x(roll);

		return (rot_z*rot_y)*rot_x; // Note: This is default execution order of C++, but just for clarity.
	}

	// -----------------------------------------------------------------------------------

	const double *getPtr() const
	{
		return (double *)v_;
	}

private:
	double v_[3][3];
};

// ---------------------------------------------------------------------------------------

// 3x3 * 3x1:
inline Matrix3x1 mtimes(const Matrix3x3 &lhs, const Matrix3x1 &rhs)
{
	return Matrix3x1(
		lhs(0,0)*rhs(0,0) + lhs(0,1)*rhs(1,0) + lhs(0,2)*rhs(2,0),
		lhs(1,0)*rhs(0,0) + lhs(1,1)*rhs(1,0) + lhs(1,2)*rhs(2,0),
		lhs(2,0)*rhs(0,0) + lhs(2,1)*rhs(1,0) + lhs(2,2)*rhs(2,0)
		);
}

inline Matrix3x1 operator*(const Matrix3x3 &lhs, const Matrix3x1 &rhs)
{
	return mtimes(lhs, rhs);
}

// 3x3 * 3x3:
inline Matrix3x3 mtimes(const Matrix3x3 &lhs, const Matrix3x3 &rhs)
{
	return Matrix3x3(
		lhs(0,0)*rhs(0,0) + lhs(0,1)*rhs(1,0) + lhs(0,2)*rhs(2,0),	lhs(0,0)*rhs(0,1) + lhs(0,1)*rhs(1,1) + lhs(0,2)*rhs(2,1),	lhs(0,0)*rhs(0,2) + lhs(0,1)*rhs(1,2) + lhs(0,2)*rhs(2,2),
		lhs(1,0)*rhs(0,0) + lhs(1,1)*rhs(1,0) + lhs(1,2)*rhs(2,0),	lhs(1,0)*rhs(0,1) + lhs(1,1)*rhs(1,1) + lhs(1,2)*rhs(2,1),	lhs(1,0)*rhs(0,2) + lhs(1,1)*rhs(1,2) + lhs(1,2)*rhs(2,2),
		lhs(2,0)*rhs(0,0) + lhs(2,1)*rhs(1,0) + lhs(2,2)*rhs(2,0),	lhs(2,0)*rhs(0,1) + lhs(2,1)*rhs(1,1) + lhs(2,2)*rhs(2,1),	lhs(2,0)*rhs(0,2) + lhs(2,1)*rhs(1,2) + lhs(2,2)*rhs(2,2)
		);
}

inline Matrix3x3 operator*(const Matrix3x3 &lhs, const Matrix3x3 &rhs)
{
	return mtimes(lhs, rhs);
}

// det(3x3)
//inline double determinant(const Matrix3x3 &v)
//{
//	double result;
//
//	result  = v_[0][0] * v_[1][1] * v_[2][2];
//	result += v_[0][1] * v_[1][2] * v_[2][0];
//	result += v_[0][2] * v_[1][0] * v_[2][1];
//	result -= v_[0][2] * v_[1][1] * v_[2][0];
//	result -= v_[0][1] * v_[1][0] * v_[2][2];
//	result -= v_[0][0] * v_[1][2] * v_[2][1];
//
//	return result;
//}

// 1/3x3:
inline Matrix3x3 inverse(const Matrix3x3 &v)
{
	Matrix3x3 result;

	result(0, 0) = v(1, 1)*v(2, 2) - v(1, 2)*v(2, 1);
	result(1, 0) = v(1, 2)*v(2, 0) - v(1, 0)*v(2, 2);
	result(2, 0) = v(1, 0)*v(2, 1) - v(1, 1)*v(2, 0);
	result(0, 1) = v(0, 2)*v(2, 1) - v(0, 1)*v(2, 2);
	result(1, 1) = v(0, 0)*v(2, 2) - v(0, 2)*v(2, 0);
	result(2, 1) = v(0, 1)*v(2, 0) - v(0, 0)*v(2, 1);
	result(0, 2) = v(0, 1)*v(1, 2) - v(0, 2)*v(1, 1);
	result(1, 2) = v(0, 2)*v(1, 0) - v(0, 0)*v(1, 2);
	result(2, 2) = v(0, 0)*v(1, 1) - v(0, 1)*v(1, 0);

	const double det = v(0, 0)*result(0, 0) + v(0, 1)*result(1, 0) + v(0, 2)*result(2, 0);

	result *= 1.0/det;

	return result;
}

// ---------------------------------------------------------------------------------------
// ---------------------------------------------------------------------------------------
// ---------------------------------------------------------------------------------------

class Matrix4x4; // forward declaration
Matrix4x4 operator*(const Matrix4x4 &lhs, const Matrix4x4 &rhs); // forward declaration

class Matrix4x4
{
public:
	Matrix4x4()
	{
	}

	Matrix4x4
		(
		double v00, double v01, double v02, double v03,
		double v10, double v11, double v12, double v13,
		double v20, double v21, double v22, double v23,
		double v30, double v31, double v32, double v33
		)
	{
		v_[0][0] = v00; v_[0][1] = v01; v_[0][2] = v02; v_[0][3] = v03;
		v_[1][0] = v10; v_[1][1] = v11; v_[1][2] = v12; v_[1][3] = v13;
		v_[2][0] = v20; v_[2][1] = v21; v_[2][2] = v22; v_[2][3] = v23;
		v_[3][0] = v30; v_[3][1] = v31; v_[3][2] = v32; v_[3][3] = v33;
	}

	// -----------------------------------------------------------------------------------

	double operator()(int col, int row) const
	{
		// (unchecked)
		return v_[col][row];
	}

	double &operator()(int col, int row)
	{
		// (unchecked)
		return v_[col][row];
	}

	// -----------------------------------------------------------------------------------

	static Matrix4x4 identity_matrix()
	{
		return Matrix4x4(1.0, 0.0, 0.0, 0.0, 
						 0.0, 1.0, 0.0, 0.0, 
						 0.0, 0.0, 1.0, 0.0, 
						 0.0, 0.0, 0.0, 1.0);
	}

	static Matrix4x4 translation_matrix(double x, double y, double z)
	{
		return Matrix4x4(1.0, 0.0, 0.0, x, 
						 0.0, 1.0, 0.0, y, 
						 0.0, 0.0, 1.0, z, 
						 0.0, 0.0, 0.0, 1.0);
	}

	static Matrix4x4 rotation_matrix_x(double angleRadians)
	{
		const double cr = cos(angleRadians);
		const double sr = sin(angleRadians);

		return Matrix4x4(1.0, 0.0, 0.0, 0.0, 
						 0.0,  cr, -sr, 0.0, 
						 0.0,  sr,  cr, 0.0, 
						 0.0, 0.0, 0.0, 1.0);
	}

	static Matrix4x4 rotation_matrix_y(double angleRadians)
	{
		const double cr = cos(angleRadians);
		const double sr = sin(angleRadians);

		return Matrix4x4( cr, 0.0,  sr, 0.0, 
						 0.0, 1.0, 0.0, 0.0, 
						 -sr, 0.0,  cr, 0.0, 
						 0.0, 0.0, 0.0, 1.0);
	}

	static Matrix4x4 rotation_matrix_z(double angleRadians)
	{
		const double cr = cos(angleRadians);
		const double sr = sin(angleRadians);

		return Matrix4x4( cr, -sr, 0.0, 0.0, 
						  sr,  cr, 0.0, 0.0, 
						 0.0, 0.0, 1.0, 0.0, 
						 0.0, 0.0, 0.0, 1.0);
	}

	static Matrix4x4 rotation_matrix_zyx(double azimuth, double elevation, double roll)
	{
		Matrix4x4 rot_z = rotation_matrix_z(azimuth);
		Matrix4x4 rot_y = rotation_matrix_y(elevation);
		Matrix4x4 rot_x = rotation_matrix_x(roll);

		return (rot_z*rot_y)*rot_x; // Note: This is default execution order of C++, but just for clarity.
	}

	// -----------------------------------------------------------------------------------

	const double *getPtr() const
	{
		return (double *)v_;
	}

private:
	double v_[4][4];
};

// ---------------------------------------------------------------------------------------

// 4x4 * 3x1:
inline Matrix3x1 mtimes(const Matrix4x4 &lhs, const Matrix3x1 &rhs)
{
	Matrix3x1 result;
	result(0, 0) = lhs(0, 0)*rhs(0, 0) + lhs(0, 1)*rhs(1, 0) + lhs(0, 2)*rhs(2, 0) + lhs(0, 3);
	result(1, 0) = lhs(1, 0)*rhs(0, 0) + lhs(1, 1)*rhs(1, 0) + lhs(1, 2)*rhs(2, 0) + lhs(1, 3);
	result(2, 0) = lhs(2, 0)*rhs(0, 0) + lhs(2, 1)*rhs(1, 0) + lhs(2, 2)*rhs(2, 0) + lhs(2, 3);
	return result;
}

inline Matrix3x1 operator*(const Matrix4x4 &lhs, const Matrix3x1 &rhs)
{
	return mtimes(lhs, rhs);
}

// 4x4 * 4x4:
inline Matrix4x4 mtimes(const Matrix4x4 &lhs, const Matrix4x4 &rhs)
{
	Matrix4x4 result;
	for (int r = 0; r < 4; ++r)
	{
		for (int c = 0; c < 4; ++c)
		{
			result(r, c) = lhs(r, 0)*rhs(0, c) + lhs(r, 1)*rhs(1, c) + lhs(r, 2)*rhs(2, c) + lhs(r, 3)*rhs(3, c);
		}
	}

	return result;
}

inline Matrix4x4 operator*(const Matrix4x4 &lhs, const Matrix4x4 &rhs)
{
	return mtimes(lhs, rhs);
}

#endif







