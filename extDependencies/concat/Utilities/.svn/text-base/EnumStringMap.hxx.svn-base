#ifndef INCLUDED_CONCAT_ENUMSTRINGMAP_HXX
#define INCLUDED_CONCAT_ENUMSTRINGMAP_HXX

#include <map>

namespace concat
{
	// two-way map for string to enum and enum to string conversions
	// also has invalid/none state which gets mapped to -1 and "invalid"
	template<typename T>
	class EnumStringMap
	{
	protected:
		static void addPair(const std::string &str, int value);

		EnumStringMap();
		virtual ~EnumStringMap();

		void fromInt(int value);
		void fromString(const char *asString);

	public:
		bool operator!() const;
		bool fail() const;

		int getAsInt() const;
		const char *getAsString() const;

	protected:
		bool isEqual(const EnumStringMap &rhs) const;
		bool isEqual(int rhs) const;

		bool failed_; // conversion failed, or not initialized
		int value_;

		static std::map<std::string, int> stringToIntLookup_;
		static std::map<int, std::string> intToStringLookup_;
		static int useCount_;
	};

	// -----------------------------------------------------------------------------------

	template<typename T>
	int EnumStringMap<T>::useCount_ = 0;

	template<typename T>
	std::map<std::string, int> EnumStringMap<T>::stringToIntLookup_;

	template<typename T>
	std::map<int, std::string> EnumStringMap<T>::intToStringLookup_;

	template<typename T>
	inline void EnumStringMap<T>::addPair(const std::string &str, int value)
	{
		if (useCount_ == 0)
		{
			stringToIntLookup_.insert(std::map<std::string, int>::value_type(str, value));
			intToStringLookup_.insert(std::map<int, std::string>::value_type(value, str));
		}
	}

	template<typename T>
	inline EnumStringMap<T>::EnumStringMap()
	{
		T::genMap();
		failed_ = true;
		value_ = -1;
		
		++useCount_;
	}

	template<typename T>
	inline EnumStringMap<T>::~EnumStringMap()
	{
		--useCount_;
		if (useCount_ == 0)
		{
			stringToIntLookup_.clear();
			intToStringLookup_.clear();
		}
	}

	template<typename T>
	inline void EnumStringMap<T>::fromInt(int value)
	{
		std::map<int, std::string>::iterator i = intToStringLookup_.find(value);
		if (i == intToStringLookup_.end())
		{
			failed_ = true;
			value_ = -1;
		}
		else
		{
			failed_ = false;
			value_ = (*i).first;
		}
	}

	template<typename T>
	inline void EnumStringMap<T>::fromString(const char *asString)
	{
		if (asString == NULL)
		{
			failed_ = true;
			value_ = -1;
			return;
		}

		std::map<std::string, int>::iterator i = stringToIntLookup_.find(asString);
		if (i == stringToIntLookup_.end())
		{
			failed_ = true;
			value_ = -1;
		}
		else
		{
			failed_ = false;
			value_ = (*i).second;
		}
	}

	template<typename T>
	inline bool EnumStringMap<T>::operator!() const
	{
		return failed_;
	}

	template<typename T>
	inline bool EnumStringMap<T>::fail() const
	{
		return failed_;
	}

	template<typename T>
	inline int EnumStringMap<T>::getAsInt() const
	{
		return value_;
	}

	template<typename T>
	inline const char *EnumStringMap<T>::getAsString() const
	{
		std::map<int, std::string>::const_iterator i = intToStringLookup_.find(value_);
		if (i == intToStringLookup_.end())
		{
			return "invalid";
		}
		else
		{
			return (*i).second.c_str();
		}
	}

	template<typename T>
	inline bool EnumStringMap<T>::isEqual(const EnumStringMap &rhs) const
	{
		if (failed_ || rhs.failed_)
			return false; // one or more not valid
		else
			return (value_ == rhs.value_);
	}

	template<typename T>
	inline bool EnumStringMap<T>::isEqual(int rhs) const
	{
		if (failed_)
			return false; // one or more not valid
		else
			return (value_ == rhs);
	}
}

// declares and defines all operators needed for convenient usage
#define ENUMSTRINGMAP_OPERATORS(class) \
	class() \
	{ \
	} \
	\
	class(const class &rhs) \
	{ \
		fromInt(rhs.getAsInt()); \
	} \
	\
	explicit class(const char *rhs) \
	{ \
		fromString(rhs); \
	} \
	\
	explicit class(int rhs) \
	{ \
		fromInt(rhs); \
	} \
	\
	class(class::PossibleValues rhs) \
	{ \
		fromInt((int)rhs); \
	} \
	\
	class &operator=(const class &rhs) \
	{ \
		fromInt(rhs.getAsInt()); \
		return *this; \
	} \
	\
	class &operator=(PossibleValues rhs) \
	{ \
		fromInt((int)rhs); \
		return *this; \
	} \
	\
	class &operator=(int rhs) \
	{ \
		fromInt(rhs); \
		return *this; \
	} \
	\
	class &operator=(const char *asString) \
	{ \
		fromString(asString); \
		return *this; \
	} \
	\
	bool operator==(PossibleValues rhs) const \
	{ \
		return isEqual((int)rhs); \
	} \
	\
	bool operator!=(PossibleValues rhs) const \
	{ \
		return !isEqual((int)rhs); \
	} \
	bool operator==(const class &rhs) const \
	{ \
		return isEqual(rhs); \
	} \
	\
	bool operator!=(const class &rhs) const \
	{ \
		return !isEqual(rhs); \
	}

#endif