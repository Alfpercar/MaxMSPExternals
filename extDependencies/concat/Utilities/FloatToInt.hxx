#ifndef INCLUDED_CONCAT_FLOATTOINT_HXX
#define INCLUDED_CONCAT_FLOATTOINT_HXX

// Faster float-to-integer conversions for platforms with slow float-to-integer conversions 
// due to having to change and reset rounding mode of fpu (most notably the x86/msvc/windows 
// combination, but possibly others as well).
// Method by Laurent de Soras, explained in the paper ``Fast Rounding of Floating Point 
// Numbers in C/C++ on Wintel Platform'' (available at http://ldesoras.free.fr).
// Method is fully accurate, but possibly the range of the input is reduced to around half 
// of the full integer range instead of full integer range (should still be more than 
// enough for most applications though).

#include <cassert> // assert()
#include <climits> // INT_MIN, INT_MAX

// assume when MSVC is used, target is x86 or x64 cpu and windows:
#if defined(_MSC_VER)
#include <cfloat> // _controlfp()
// other cases:
#else
#include <cmath> // floor(), ceil()
#include <cstdlib> // rint()
#endif

// TODO: For XCode 1.5 (?), bring rint() into std namespace (so std::rint() can be used instead of rint()). Check if still needed with newer versions of XCode.
// TODO: Check if fix is also needed for x64 cpus.
// TODO: Check if fix is also needed for x86 cpus on non-windows os'es. (if so, implement using gcc-style asm as well)
// TOOD: Check if fix is also needed for XCode for intel platform.
// XXX: maybe using nearbyint() (C99) instead of rint(), rint() may throw FE_INEXACT exception; tho it seems nearbyint() rounds using current prevailing rounding mode

namespace concat
{
	// float-to-int:
#if defined(_MSC_VER)
	inline int round_int(float x)
	{
		assert((_controlfp(0, 0) & _MCW_RC) == _RC_NEAR); // check rounding-mode
		assert(x > static_cast<float>(INT_MIN/2) - 1.f);
		assert(x < static_cast<float>(INT_MAX/2) + 1.f);

		static const float round_to_nearest = +0.5f;
		int i;

		__asm
		{
			fld   x
			fadd  st, st (0)
			fadd  round_to_nearest
			fistp i
			sar   i, 1
		}

		return i;
	}
#else
	inline int round_int(float x)
	{
		assert(x > static_cast<float>(INT_MIN/2) - 1.f);
		assert(x < static_cast<float>(INT_MAX/2) + 1.f);
		return static_cast<int>(std::rint(x));
	}
#endif

	// double-to-int:
#if defined(_MSC_VER)
	inline int round_int(double x)
	{
		assert((_controlfp(0, 0) & _MCW_RC) == _RC_NEAR); // check rounding-mode
		assert(x > static_cast<double>(INT_MIN/2) - 1.0);
		assert(x < static_cast<double>(INT_MAX/2) + 1.0);

		static const double round_to_nearest = +0.5;
		int i;

		__asm
		{
			fld   x
			fadd  st, st (0)
			fadd  round_to_nearest
			fistp i
			sar   i, 1
		}

		return i;
	}
#else
	inline int round_int(double x)
	{
		assert(x > static_cast<double>(INT_MIN/2) - 1.0);
		assert(x < static_cast<double>(INT_MAX/2) + 1.0);
		return static_cast<int>(std::rint(x));
	}
#endif

// ---------------------------------------------------------------------------------------

	// float-to-int:
#if defined(_MSC_VER)
	inline int floor_int(float x)
	{
		assert((_controlfp(0, 0) & _MCW_RC) == _RC_NEAR); // check rounding-mode
		assert(x > static_cast<float>(INT_MIN/2) - 1.f);
		assert(x < static_cast<float>(INT_MAX/2) + 1.f);

		static const float round_towards_m_i = -0.5f;
		int i;

		__asm
		{
			fld   x
			fadd  st, st (0)
			fadd  round_towards_m_i
			fistp i
			sar   i, 1
		}
		
		return i;
	}
#else
	inline int floor_int(float x)
	{
		assert(x > static_cast<float>(INT_MIN/2) - 1.f);
		assert(x < static_cast<float>(INT_MAX/2) + 1.f);
		return static_cast<int>(std::floor(x));
	}
#endif

	// double-to-int:
#if defined(_MSC_VER)
	inline int floor_int(double x)
	{
		assert((_controlfp(0, 0) & _MCW_RC) == _RC_NEAR); // check rounding-mode
		assert(x > static_cast<double>(INT_MIN/2) - 1.0);
		assert(x < static_cast<double>(INT_MAX/2) + 1.0);

		static const double round_towards_m_i = -0.5;
		int i;

		__asm
		{
			fld   x
			fadd  st, st (0)
			fadd  round_towards_m_i
			fistp i
			sar   i, 1
		}

		return i;
	}
#else
	inline int floor_int(float x)
	{
		assert(x > static_cast<double>(INT_MIN/2) - 1.0);
		assert(x < static_cast<double>(INT_MAX/2) + 1.0);
		return static_cast<int>(std::floor(x));
	}
#endif

// ---------------------------------------------------------------------------------------

	// float-to-int:
#if defined(_MSC_VER)
	inline int ceil_int(float x)
	{
		assert((_controlfp(0, 0) & _MCW_RC) == _RC_NEAR); // check rounding-mode
		assert(x > static_cast<float>(INT_MIN/2) - 1.f);
		assert(x < static_cast<float>(INT_MAX/2) + 1.f);

		static const float round_towards_p_i = -0.5f;
		int i;

		__asm
		{
			fld   x
			fadd  st, st (0)
			fsubr round_towards_p_i
			fistp i
			sar   i, 1
		}

		return (-i);
	}
#else
	inline int ceil_int(double x)
	{
		assert(x > static_cast<float>(INT_MIN/2) - 1.f);
		assert(x < static_cast<float>(INT_MAX/2) + 1.f);
		return static_cast<int>(std::ceil(x));
	}
#endif

	// double-to-int:
#if defined(_MSC_VER)
	inline int ceil_int(double x)
	{
		assert((_controlfp(0, 0) & _MCW_RC) == _RC_NEAR); // check rounding-mode
		assert(x > static_cast<double>(INT_MIN/2) - 1.0);
		assert(x < static_cast<double>(INT_MAX/2) + 1.0);

		static const double round_towards_p_i = -0.5;
		int i;

		__asm
		{
			fld   x
			fadd  st, st (0)
			fsubr round_towards_p_i
			fistp i
			sar   i, 1
		}

		return (-i);
	}
#else
	inline int ceil_int(double x)
	{
		assert(x > static_cast<double>(INT_MIN/2) - 1.0);
		assert(x < static_cast<double>(INT_MAX/2) + 1.0);
		return static_cast<int>(std::ceil(x));
	}
#endif
}

#endif // INCLUDED_CONCAT_FLOATTOINT_HXX

