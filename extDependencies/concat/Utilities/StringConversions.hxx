#ifndef INCLUDED_CONCAT_STRINGCONVERSIONS_HXX
#define INCLUDED_CONCAT_STRINGCONVERSIONS_HXX

#include <string>
#include <sstream>

namespace concat
{
	// Utility class, use free functions instead.
	template<typename T>
	class ConvertToString
	{
	public:
		ConvertToString(const T &v)
		{
			conv_ << v;
			conv_ >> result_;
		}

		bool fail() const
		{
			return conv_.fail();
		}

		const std::string &result() const
		{
			return result_;
		}

	private:
		std::stringstream conv_;
		std::string result_;
	};

	template<typename T>
	class ConvertFromString
	{
	public:
		ConvertFromString(const std::string &str)
		{
			conv_ << str;
			conv_ >> result_;
		}

		bool fail() const
		{
			return conv_.fail();
		}

		const T &result() const
		{
			return result_;
		}

	private:
		std::stringstream conv_;
		T result_;
	};

	// -----------------------------------------------------------------------------------

	inline std::string intToString(int v)
	{
		ConvertToString<int> conv(v);
		return conv.result();
	}

	inline bool intToStringValid(int v)
	{
		ConvertToString<int> conv(v);
		return (!conv.fail());
	}

	inline int intFromString(std::string str)
	{
		ConvertFromString<int> conv(str);
		return conv.result();
	}

	inline bool intFromStringValid(std::string str)
	{
		ConvertFromString<int> conv(str);
		return (!conv.fail());
	}

	// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

	inline std::string floatToString(float v)
	{
		ConvertToString<float> conv(v);
		return conv.result();
	}

	inline bool floatToStringValid(float v)
	{
		ConvertToString<float> conv(v);
		return (!conv.fail());
	}

	inline float floatFromString(std::string str)
	{
		ConvertFromString<float> conv(str);
		return conv.result();
	}

	inline bool floatFromStringValid(std::string str)
	{
		ConvertFromString<float> conv(str);
		return (!conv.fail());
	}

	// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

	inline std::string doubleToString(double v)
	{
		ConvertToString<double> conv(v);
		return conv.result();
	}

	inline bool doubleToStringValid(double v)
	{
		ConvertToString<double> conv(v);
		return (!conv.fail());
	}

	inline double doubleFromString(std::string str)
	{
		ConvertFromString<double> conv(str);
		return conv.result();
	}

	inline bool doubleFromStringValid(std::string str)
	{
		ConvertFromString<double> conv(str);
		return (!conv.fail());
	}
}

#endif