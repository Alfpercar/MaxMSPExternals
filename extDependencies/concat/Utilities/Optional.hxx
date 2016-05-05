#ifndef INCLUDED_CONCAT_OPTIONAL_HXX
#define INCLUDED_CONCAT_OPTIONAL_HXX

#include <cstddef>
#include <cassert>

namespace concat
{
	// Adds an explicit 'uninitialized' state to values that don't have an inherit invalid 
	// state to allow easy checking whether a value has been initialized or not.
	//
	// Loosely follows boost::optional<T>, but simplified with the following restrictions:
	// T can't be a reference type.
	// Optional<T> always wraps a copy of a value, thus changing the original value has no 
	// effect on the Optional<T> copy.
	// Conversions from Optional<U> (i.e. of different wrapped type) are not supported.
	// Comparisons other than equal or not equal aren't supported.
	// Swapping Optionals isn't supported.
	template<typename T>
	class Optional
	{
	public:
		enum UninitizedType
		{
			UNINITIALIZED
		};

		Optional();
		explicit Optional(const T &value);
		Optional(const Optional<T> &other);

		Optional &operator=(const T &value);
		Optional &operator=(UninitizedType); // un-initialize
		Optional &operator=(const Optional<T> &other);

		const T &operator*() const;
		T &operator*();
		const T &get() const;
		T &get();

		const T *getPointer() const;
		T *getPointer();

		const T *operator->() const;
		T *operator->();

		bool operator!() const;
		bool isInitialized() const;

		bool operator==(const Optional<T> &other) const;
		bool operator!=(const Optional<T> &other) const;

	private:
		T value_;
		bool isInitialized_;
	};
}

// ---------------------------------------------------------------------------------------

namespace concat
{
	template<typename T>
	inline Optional<T>::Optional() : isInitialized_(false)
	{
	}

	template<typename T>
	inline Optional<T>::Optional(const T &value) : value_(value), isInitialized_(true)
	{
	}

	template<typename T>
	inline Optional<T>::Optional(const Optional<T> &other)
	{
		if (other.isInitialized_)
		{
			value_ = other.value_;
			isInitialized_ = true;
		}
		else
		{
			isInitialized_ = false;
		}
	}

	template<typename T>
	inline Optional<T> &Optional<T>::operator=(const T &value)
	{
		value_ = value;
		isInitialized_ = true;
		return *this;
	}

	template<typename T>
	inline Optional<T> &Optional<T>::operator=(UninitizedType)
	{
		isInitialized_ = false;
		return *this;
	}

	template<typename T>
	inline Optional<T> &Optional<T>::operator=(const Optional<T> &other)
	{
		if (other.isInitialized_)
		{
			value_ = other.value_;
			isInitialized_ = true;
		}
		else
		{
			isInitialized_ = false;
		}

		return *this;
	}

	template<typename T>
	inline const T &Optional<T>::operator*() const
	{
		assert(isInitialized_);
		return value_;
	}

	template<typename T>
	inline T &Optional<T>::operator*()
	{
		assert(isInitialized_);
		return value_;
	}

	template<typename T>
	inline const T &Optional<T>::get() const
	{
		return operator*();
	}

	template<typename T>
	inline T &Optional<T>::get()
	{
		return operator*();
	}

	template<typename T>
	inline const T *Optional<T>::getPointer() const
	{
		if (isInitialized_)
			return &value_;
		else
			return NULL;
	}

	template<typename T>
	inline T *Optional<T>::getPointer()
	{
		if (isInitialized_)
			return &value_;
		else
			return NULL;
	}

	template<typename T>
	inline const T *Optional<T>::operator->() const
	{
		assert(isInitialized_);
		return &value_;
	}

	template<typename T>
	inline T *Optional<T>::operator->()
	{
		assert(isInitialized_);
		return &value_;
	}

	template<typename T>
	inline bool Optional<T>::operator!() const
	{
		return (!isInitialized_);
	}

	template<typename T>
	inline bool Optional<T>::isInitialized() const
	{
		return (isInitialized_);
	}

	template<typename T>
	inline bool Optional<T>::operator==(const Optional<T> &other) const
	{
		if (this->isInitialized_ && other.isInitialized_)
		{
			return (this->value_ == other_.value_);
		}
		else if (!this->isInitialized_ && !other.isInitialized_)
		{
			return true;
		}
		else
		{
			return false;
		}
	}

	template<typename T>
	inline bool Optional<T>::operator!=(const Optional<T> &other) const
	{
		return !(*this == other);
	}
}

#endif