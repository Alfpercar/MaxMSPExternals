#ifndef INCLUDED_CONCAT_STRINGHELPERS_HXX
#define INCLUDED_CONCAT_STRINGHELPERS_HXX

#include <functional> // binary_function
#include <cctype> // tolower
#include <string>
#include <algorithm>
#include <cstdarg>
#include <tchar.h>

namespace concat
{
	void strToLower(std::string &s);

	// utility to do fprint style formatting to std::string
	// XXX: perhaps replace with many overloads of upto 4 arguments with int, float, string, pointer (?)
	// or perhaps make template function
	inline std::string formatStr(const char *format, ...)
	{
		va_list args;
		int len;
		char *buffer;

		va_start(args, format);

		len = _vscprintf(format, args) + 1; // incl. terminating '\0'
		buffer = new char[len];
		vsprintf(buffer, format, args);
		std::string result = buffer;
		delete[] buffer;

		va_end(args);

		return result;
	}

	// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

	template<typename CharT>
	struct CaseInsensitiveEqual : public std::binary_function<CharT, CharT, bool>
	{
		bool operator()(CharT char1, CharT char2) const
		{
			return (tolower(char1) == tolower(char2));
		}
	};

	inline bool ciStrEqual(const std::string &str1, const std::string &str2)
	{
		std::string s1 = str1;
		std::string s2 = str2;
		strToLower(s1);
		strToLower(s2);
		return (s1 == s2);
		//std::pair<std::string::const_iterator, std::string::const_iterator> result = std::mismatch(str1.begin(), str1.end(), str2.begin(), CaseInsensitiveEqual<std::string::value_type>());

		//return (result.first == str1.end() && result.second == str2.end());
	}

	// -----------------------------------------------------------------------------------

	// replace all occurrences of a in s with b
	inline void strReplaceAll(std::string &s, const std::string &a, const std::string &b)
	{
		std::string::size_type pos = 0;
		
		while (1)
		{
			pos = s.find(a, pos);

			if (pos == std::string::npos)
				break; // done

			s.replace(pos, a.size(), b);

			pos += a.size();
		}
	}

	// -----------------------------------------------------------------------------------

	inline void strToLower(std::string &s)
	{
		std::transform(s.begin(), s.end(), s.begin(), (int(*)(int))std::tolower);
	}

	inline void strToUpper(std::string &s)
	{
		std::transform(s.begin(), s.end(), s.begin(), (int(*)(int))std::toupper);
	}

	// -----------------------------------------------------------------------------------

	inline std::string stripWhitespace(const std::string &s)
	{
		std::string result = s;
		strReplaceAll(result, " ", "");
		strReplaceAll(result, "\t", "");
		return result;
	}

	// -----------------------------------------------------------------------------------

	inline std::string stripLeadingTrailingWhitespace(const std::string &s)
	{
		std::string result = s;

		std::string::size_type nonWhitespaceBegin = result.find_first_not_of(" \t");
		if (nonWhitespaceBegin != std::string::npos)
			result = result.substr(nonWhitespaceBegin, result.size() - nonWhitespaceBegin);

		std::string::size_type nonWhiteSpaceEnd = result.find_last_not_of(" \t");
		if (nonWhiteSpaceEnd != std::string::npos)
			result = result.substr(0, nonWhiteSpaceEnd+1);

		return result;
	}

	// -----------------------------------------------------------------------------------

	inline std::string stripQuotationMarks(const std::string &s)
	{
		std::string result = s;

		if (s.size() >= 2 && s[0] == '\"' && s[s.size()-1] == '\"')
			result = s.substr(1, s.size()-2);

		return result;
	}


}

#endif

