#ifndef INCLUDED_CONCAT_BINFILE_HXX
#define INCLUDED_CONCAT_BINFILE_HXX

#include <cstddef>
#include <cassert>
#include <fstream>

namespace concat
{
	// loads raw binary file into memory
	class BinFile
	{
	public:
		BinFile();
		BinFile(const char *filename);
		~BinFile();

		void load(const char *filename);
		void unload(); // deallocate memory, also done from deconstructor

		bool fail() const;

		int getNumBytes() const;
		int getNumItemsFloats() const;
		int getNumItemsDoubles() const;

		const char *getBytePointer() const;
		const float *getFloatPointer() const;
		const double *getDoublePointer() const;

	private:
		char *data_;
		int size_; // bytes
	};
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

namespace concat
{
	inline BinFile::BinFile()
	{
		data_ = NULL;
		size_ = 0;
	}

	inline BinFile::BinFile(const char *filename)
	{
		data_ = NULL;
		size_ = 0;

		load(filename);
	}

	inline BinFile::~BinFile()
	{
		unload();
	}

	// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
	
	inline void BinFile::load(const char *filename)
	{
		BinFile::~BinFile();

		std::ifstream file;
		file.open(filename, std::ifstream::in);

		if (!file)
			return;

		file.seekg(0, std::ios_base::end);
		int size = (int)file.tellg();

		if (size == 0)
			return;

		data_ = new char[size];
		size_ = size;

		file.seekg(0, std::ios_base::beg);
		file.read(data_, size_);

		file.close();
	}

	inline void BinFile::unload()
	{
		delete[] data_;
		data_ = NULL;
		size_ = 0;
	}

	inline bool BinFile::fail() const
	{
		return (data_ == NULL);
	}

	// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

	inline int BinFile::getNumBytes() const
	{
		return size_;
	}
	
	inline int BinFile::getNumItemsFloats() const
	{
		assert((size_ % sizeof(float)) == 0);
		return size_/sizeof(float);
	}

	inline int BinFile::getNumItemsDoubles() const
	{
		assert((size_ % sizeof(double)) == 0);
		return size_/sizeof(double);
	}

	inline const char *BinFile::getBytePointer() const
	{
        return data_;
	}

	inline const float *BinFile::getFloatPointer() const
	{
		return (const float *)data_;
	}

	inline const double *BinFile::getDoublePointer() const
	{
		return (const double *)data_;
	}
}

#endif