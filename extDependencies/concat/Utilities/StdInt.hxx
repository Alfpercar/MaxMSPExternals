#ifndef INCLUDED_CONCAT_STDINT_HXX
#define INCLUDED_CONCAT_STDINT_HXX

namespace concat
{
#if defined(_MSC_VER) && defined(_WIN32)
	// MSVC/X86-32:
	typedef unsigned char byte;

	typedef char int8_t;
	typedef char int_least8_t;
	typedef char int_fast8_t;
	typedef unsigned char uint8_t;
	typedef unsigned char uint_least8_t;
	typedef unsigned char uint_fast8_t;

	typedef short int16_t;
	typedef short int_least16_t;
	typedef short int_fast16_t;
	typedef unsigned short uint16_t;
	typedef unsigned short uint_least16_t;
	typedef unsigned short uint_fast16_t;

	typedef long int32_t;
	typedef long int_least32_t;
	typedef long int_fast32_t;
	typedef unsigned long uint32_t;
	typedef unsigned long uint_least32_t;
	typedef unsigned long uint_fast32_t;

	typedef __int64 int64_t;
	typedef __int64 int_least64_t;
	typedef __int64 int_fast64_t;
	typedef unsigned __int64 uint64_t;
	typedef unsigned __int64 uint_least64_t;
	typedef unsigned __int64 uint_fast64_t;

	typedef int64_t intmax_t;
	typedef uint64_t uintmax_t;

//#elif defined(???)
//	// XCODE/G4:
//	typedef unsigned char byte;
//
//	typedef char int8_t;
//	typedef char int_least8_t;
//	typedef char int_fast8_t;
//	typedef unsigned char uint8_t;
//	typedef unsigned char uint_least8_t;
//	typedef unsigned char uint_fast8_t;
//
//	typedef short int16_t;
//	typedef short int_least16_t;
//	typedef short int_fast16_t;
//	typedef unsigned short uint16_t;
//	typedef unsigned short uint_least16_t;
//	typedef unsigned short uint_fast16_t;
//
//	typedef long int32_t;
//	typedef long int_least32_t;
//	typedef long int_fast32_t;
//	typedef unsigned long uint32_t;
//	typedef unsigned long uint_least32_t;
//	typedef unsigned long uint_fast32_t;
//
//	typedef long long int64_t;
//	typedef long long int_least64_t;
//	typedef long long int_fast64_t;
//	typedef unsigned long long uint64_t;
//	typedef unsigned long long uint_least64_t;
//	typedef unsigned long long uint_fast64_t;
//	
#else
	// Unsupported platform:
	#error No fixed-width integers defined for platform (StdInt.hxx).
#endif
}

#endif // INCLUDED_DAISY_STDINT_HXX