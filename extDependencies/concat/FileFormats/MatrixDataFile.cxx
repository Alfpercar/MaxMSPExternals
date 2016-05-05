#include "MatrixDataFile.hxx"

namespace concat
{
	uint32_t headerSize()
	{
		uint32_t headerSize = 0;

		// id : 4 chars
		headerSize += 4;
		// sample rate : double
		headerSize += sizeof(double);
		// hop size : uint32
		headerSize += sizeof(uint32_t);
		// number of values per frame : uint32
		headerSize += sizeof(uint32_t);
		// number of frames : uint32
		headerSize += sizeof(uint32_t);
		// datasize : uint32
		headerSize += sizeof(uint32_t);

		return headerSize;
	}
}

// ---------------------------------------------------------------------------------------

namespace concat
{
	MatrixDataFileRead::MatrixDataFileRead()
	{
		initState();
	}

	MatrixDataFileRead::MatrixDataFileRead(const char *filename)
	{
		initState();
		open(filename);
	}

	MatrixDataFileRead::~MatrixDataFileRead()
	{
		close();
	}

	void MatrixDataFileRead::initState()
	{
		readPosFrames_ = 0;
		sampleRate_ = 0.0;
		hopSize_ = 0;
		numValuesPerFrame_ = 0;
		numFrames_ = 0;
		dataSizeFloats_ = 0;
		offsetTable_ = NULL;
		sizeTable_ = NULL;
		isOk_ = false;
	}

	// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

	void MatrixDataFileRead::open(const char *filename)
	{
		if (stream_.is_open())
			return;

		if (filename == NULL || *filename == '\0')
		{
			initState(); // also sets isOk_ = false
			return;
		}

		// open file stream:
		stream_.open(filename, std::ios_base::binary);

		if (!stream_.is_open() || (!stream_))
		{
			initState(); // also sets isOk_ = false
			return; // error: couldn't open file
		}

		// read header:
		if (remainingBytes() < headerSize())
		{
			initState(); // also sets isOk_ = false
			stream_.close();
			return; // error: file exists, but doesn't contain enough bytes to contain header (invalid file)
		}

		char id[4];
		stream_.read(&id[0], 1);
		stream_.read(&id[1], 1);
		stream_.read(&id[2], 1);
		stream_.read(&id[3], 1);

		if (id[0] != 'M' || id[1] != 'T' || id[2] != 'R' || id[3] != 'X')
		{
			initState(); // also sets isOk_ = false
			stream_.close();
			return; // error: file does not have correct four char id
		}

		stream_.read((char *)&sampleRate_, sizeof(double));
		if (sampleRate_ <= 0.0 || sampleRate_ > 192000.0)
		{
			initState(); // also sets isOk_ = false
			stream_.close();
			return; // error: sample rate invalid
		}

		stream_.read((char *)&hopSize_, sizeof(uint32_t));
		if (/*hopSize_ < 0 || */hopSize_ > 65536)
		{
			initState(); // also sets isOk_ = false
			stream_.close();
			return; // error: hop size invalid
		}

		stream_.read((char *)&numValuesPerFrame_, sizeof(uint32_t));
		stream_.read((char *)&numFrames_, sizeof(uint32_t));
/*		if (numValuesPerFrame_ < 0 || numFrames_ < 0)
		{
			initState(); // also sets isOk_ = false
			stream_.close();
			return; // error: matrix dimensions invalid
		}*/

		stream_.read((char *)&dataSizeFloats_, sizeof(uint32_t));
		if (dataSizeFloats_ < 0)
		{
			initState(); // also sets isOk_ = false
			stream_.close();
			return; // error: data size invalid
		}

		// read offset and sizes table:
		if (hasNonConstantNumValuesPerFrame() && getNumFrames() > 0)
		{
			std::streampos cur = stream_.tellg();
			assert(cur == (std::streampos)headerSize());

			// skip to table data:
			stream_.seekg(dataSizeFloats_*sizeof(float), std::ios_base::cur);
			assert(remainingBytes() == numFrames_*sizeof(uint32_t));
			if (remainingBytes() < numFrames_*sizeof(uint32_t))
			{
				initState(); // also sets isOk_ = false
				stream_.close();
				return; // error: table size invalid
			}

			assert(offsetTable_ == NULL);
			assert(sizeTable_ == NULL);
			offsetTable_ = new uint32_t[numFrames_+1];
			sizeTable_ = new uint32_t[numFrames_];

			// read size table:
			stream_.read((char *)(&(sizeTable_[0])), numFrames_*sizeof(uint32_t));

			// check size table:
			uint32_t totalSize = 0;
			for (int i = 0; i < getNumFrames(); ++i)
			{
				totalSize += sizeTable_[i];

				if (totalSize > dataSizeFloats_)
				{
					delete[] offsetTable_;
					delete[] sizeTable_;
					initState(); // also sets isOk_ = false
					stream_.close();
					return; // error: size table invalid
				}
			}

			// compute offset table:
			offsetTable_[0] = 0;
			for (uint32_t i = 1; i < numFrames_+1; ++i)
			{
				offsetTable_[i] = offsetTable_[i-1] + sizeTable_[i-1];
			}

			assert(remainingBytes() == 0);

			stream_.seekg(cur, std::ios_base::beg);
		}

		isOk_ = true;
	}

	void MatrixDataFileRead::close()
	{
		if (!isOpen())
			return;

		stream_.close();
		delete[] offsetTable_;
		delete[] sizeTable_;
		// (offsetTable_ and sizeTable_ are set to NULL in initState())

		initState();
	}

	bool MatrixDataFileRead::isOpen() const
	{
		return stream_.is_open();
	}

	bool MatrixDataFileRead::isOk() const
	{
		return isOk_;
	}

	// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

	double MatrixDataFileRead::getSampleRate() const
	{
		return sampleRate_;
	}

	uint32_t MatrixDataFileRead::getHopSize() const
	{
		return hopSize_;
	}

	bool MatrixDataFileRead::hasNonConstantFrameRate() const
	{
		return (hopSize_ == 0);
	}

	uint32_t MatrixDataFileRead::getNumValuesPerFrame() const
	{
		return numValuesPerFrame_;
	}

	bool MatrixDataFileRead::hasNonConstantNumValuesPerFrame() const
	{
		return (numValuesPerFrame_ == 0);
	}
	
	uint32_t MatrixDataFileRead::getNumFrames() const
	{
		return numFrames_;
	}

	uint32_t MatrixDataFileRead::getDataSizeFloats() const
	{ 
		return dataSizeFloats_;
	}

	// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

	int MatrixDataFileRead::read(float *data, int sizeFrames)
	{
		assert(isOpen() && isOk_ && sizeFrames >= 0);
		if (!isOpen() || !isOk_)
			return 0;

		// XXX: here same as computeReadTargetSizeFloats() except for int vs uint32 stuff -->
		if (sizeFrames <= 0)
			return 0;

		// limit sizeFrames if it's greater than the number of frames available:
		// NOTE: cast to uint32_t is save because above already returns if sizeFrames < 0
		if ((uint32_t)sizeFrames > remainingFrames())
			sizeFrames = remainingFrames();

		// computed required target memory:
		int numFloatsToRead;
		if (hasNonConstantNumValuesPerFrame())
		{
			numFloatsToRead = 0;
			for (uint32_t i = readPosFrames_; i < readPosFrames_ + sizeFrames; ++i)
			{
				numFloatsToRead += sizeTable_[i];
			}
		}
		else
		{
			numFloatsToRead = sizeFrames*getNumValuesPerFrame();
		}
		// <------------------------------------------------------------------------------

		// do read:
		if (data != NULL)
		{
			stream_.read((char *)data, numFloatsToRead*sizeof(float));
			readPosFrames_ += sizeFrames; // XXX: added 11/06/2009, why was this not here?!
		}

		return numFloatsToRead;
	}

	uint32_t MatrixDataFileRead::tellFrames() const
	{
        return readPosFrames_;
	}

	void MatrixDataFileRead::seekFromBegin(uint32_t offsetFrames)
	{
		assert(isOpen() && isOk_);
		assert(offsetFrames >= 0 && offsetFrames < numFrames_);

		// seek to correct offset:
		uint32_t offsetBytes;
		if (hasNonConstantNumValuesPerFrame())
			offsetBytes = offsetTable_[offsetFrames]*sizeof(float);
		else
			offsetBytes = offsetFrames*numValuesPerFrame_*sizeof(float);

		stream_.seekg(headerSize() + offsetBytes, std::ios::beg);
		readPosFrames_ = offsetFrames;
	}

/*	int MatrixDataFileRead::readNonSeq(float *data, int offsetFrames, int sizeFrames)
	{
		assert(isOpen() && isOk_);
		assert(offsetFrames >= 0 && offsetFrames < numFrames_);
		assert(sizeFrames >= 0 && sizeFrames <= numFrames_ - offsetFrames);

		unsigned int curReadPosBytes = stream_.tellg();
		int curReadPosFrames = readPosFrames_;

		// seek to correct offset:
		unsigned int offsetBytes;
		if (hasNonConstantNumValuesPerFrame())
			offsetBytes = offsetTable_[offsetFrames]*sizeof(float);
		else
			offsetBytes = offsetFrames*numValuesPerFrame_*sizeof(float);
		stream_.seekg(headerSize() + offsetBytes, std::ios::beg);
		readPosFrames_ = offsetFrames;
		
		// do sequential read:
		int result = read(data, sizeFrames);

		// reset read position (isn't modified):
		stream_.seekg(curReadPosBytes, std::ios_base::beg);
		readPosFrames_ = curReadPosFrames;

		return result;
	}*/

	// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

	const uint32_t *MatrixDataFileRead::getSizeTable() const
	{
		return sizeTable_;
	}

	const uint32_t *MatrixDataFileRead::getOffsetTable() const
	{
		return offsetTable_;
	}

	// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

	uint32_t MatrixDataFileRead::remainingBytes()
	{
		std::streampos cur = stream_.tellg();
		stream_.seekg(0, std::ios::end);
		uint32_t remaining = (unsigned int)stream_.tellg() - (unsigned int)cur;
		stream_.seekg(cur);
		return remaining;
	}

	uint32_t MatrixDataFileRead::remainingFrames()
	{
		uint32_t remainingFrames = 0;
		uint32_t rb = remainingBytes();
		for (uint32_t i = readPosFrames_; i < numFrames_; ++i)
		{
			uint32_t frameSizeBytes;
			if (hasNonConstantNumValuesPerFrame() && sizeTable_ != NULL)
				frameSizeBytes = sizeTable_[i]*sizeof(float);
			else
				frameSizeBytes = numValuesPerFrame_*sizeof(float);

			assert(frameSizeBytes >= 0);

			rb -= frameSizeBytes;
			if (rb >= 0)
			{
				// there were enough bytes in the file to entirely read this frame
				++remainingFrames;
			}
			else
			{
				// eof
				break;
			}
		}

		assert(remainingFrames == numFrames_ - readPosFrames_); // normally the case, unless the file is damaged
		return remainingFrames;
	}
}

// ---------------------------------------------------------------------------------------

namespace concat
{
	MatrixDataFileWrite::MatrixDataFileWrite()
	{
		initState();
	}

	MatrixDataFileWrite::MatrixDataFileWrite(const char *filename, double sampleRate, int hopSize, int numValuesPerFrame)
	{
		initState();
		open(filename, sampleRate, hopSize, numValuesPerFrame);
	}

	MatrixDataFileWrite::~MatrixDataFileWrite()
	{
		close();
	}

	void MatrixDataFileWrite::initState()
	{
		sampleRate_ = 0.0;
		hopSize_ = 0;
		numValuesPerFrame_ = 0;
		numFrames_ = 0;
		dataSizeFloats_ = 0;
	}

	// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

	void MatrixDataFileWrite::writeHeader()
	{
		std::streampos cur = stream_.tellp();
		stream_.seekp(0, std::ios_base::beg);
		std::streampos beg = stream_.tellp();
		assert(cur == beg);

		// write four char identifier:
		char id[4];
		id[0] = 'M';
		id[1] = 'T';
		id[2] = 'R';
		id[3] = 'X';
		stream_.write(&id[0], 1);
		stream_.write(&id[1], 1);
		stream_.write(&id[2], 1);
		stream_.write(&id[3], 1);

		// write sample rate and frame size:
		stream_.write((char *)&sampleRate_, sizeof(double));
		stream_.write((char *)&hopSize_, sizeof(uint32_t));

		// write matrix dimensions:
		stream_.write((char *)&numValuesPerFrame_, sizeof(uint32_t));
		stream_.write((char *)&numFrames_, sizeof(uint32_t));
		stream_.write((char *)&dataSizeFloats_, sizeof(uint32_t));

		assert(stream_.tellp() == (std::streampos)headerSize());
		stream_.seekp(cur, std::ios_base::beg);
	}

	void MatrixDataFileWrite::writeSizeTable()
	{
		assert(numValuesPerFrame_ == 0);

		std::streampos cur = stream_.tellp();
		stream_.seekp(0, std::ios_base::end);
		std::streampos end = stream_.tellp();

		assert(cur == end);

		uint32_t n = (uint32_t)sizeTable_.size();
		for (uint32_t i = 0; i < n; ++i)
		{
			uint32_t numValuesPerFrame = sizeTable_[i];
			stream_.write((char *)&numValuesPerFrame, sizeof(uint32_t));
		}

//		stream_.seekp(cur, std::ios_base::beg);
	}

	// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

	void MatrixDataFileWrite::open(const char *filename, double sampleRate, int hopSize, int numValuesPerFrame)
	{
		assert(numValuesPerFrame >= 0);
		if (numValuesPerFrame < 0)
			numValuesPerFrame = 0;

		if (isOpen())
		{
			return;
		}

		// open file stream:
		stream_.open(filename, std::ios_base::trunc | std::ios_base::binary);

		if (!stream_.is_open() || (!stream_))
		{
			assert(0); // safe to ignore, opening failed
			return;
		}

		assert(sizeTable_.empty());

		sampleRate_ = sampleRate;
		hopSize_ = hopSize;
		numValuesPerFrame_ = numValuesPerFrame;
		numFrames_ = 0;
		dataSizeFloats_ = 0;
		writeHeader();
		stream_.seekp(headerSize(), std::ios_base::beg);
	}

	void MatrixDataFileWrite::close()
	{
		if (!isOpen())
			return;

		// update header (to update num frames and data size floats fields):
		std::streampos cur = stream_.tellp();
		stream_.seekp(0, std::ios_base::beg);
		writeHeader();
		stream_.seekp(cur, std::ios_base::beg);

		// write size table:
		if (numValuesPerFrame_ == 0)
			writeSizeTable();
		stream_.flush();

		// close file:
		stream_.close();

		// initialize internal state for re-open:
		sizeTable_.erase(sizeTable_.begin(), sizeTable_.end());
		initState();
	}

	bool MatrixDataFileWrite::isOpen() const
	{
		return stream_.is_open();
	}

	// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

	void MatrixDataFileWrite::write(const float *data, int sizeFrames)
	{
		assert(data != NULL);
		assert(isOpen());
		assert(sizeFrames > 0);
		assert(numValuesPerFrame_ > 0);

		if (data == NULL || !isOpen() || sizeFrames <= 0 || numValuesPerFrame_ <= 0)
			return;

		//std::streampos prev = stream_.tellp();
		//int prevNumRows = numRows_;

		// write data:
		stream_.write((const char *)data, sizeFrames*numValuesPerFrame_*sizeof(float));

		assert((!stream_) == false); // XXX: this happens when disc full (and in debug mode)!!! should handle differently
		//if (!stream_)
		//{
		//	assert(0); // safe to ignore, writing failed
		//	return;
		//}
		// XXX: handle case of only being able to write partially (disk full, etc)

		// update internal state:
		numFrames_ += sizeFrames;
		dataSizeFloats_ += sizeFrames*numValuesPerFrame_;

		// Note: Header is _NOT_ updated (instead done on close()).

		//// updating header failed, revert object state to before write: 
		//// note that the file will contain the written data and may be corrupt XXXX: <- or not? seekp() and flush()/close() seems to truncate to last write pos on win32
		//if (!stream_)
		//{
		//	stream_.seekp(prev);
		//	numRows_ = prevNumRows;

		//	assert(0); // safe to ignore (file data may be corrupt), failing write
		//	return;
		//}
	}

	void MatrixDataFileWrite::writeSingleNonConstSizeFrame(const float *data, int numValuesPerFrame)
	{
		assert(data != NULL || numValuesPerFrame == 0);
		assert(isOpen());
		assert(numValuesPerFrame >= 0);

		if ((data == NULL && numValuesPerFrame != 0) || !isOpen() || numValuesPerFrame < 0)
			return;

		// write data:
		if (numValuesPerFrame != 0) // (allow zero size frames)
			stream_.write((const char *)data, numValuesPerFrame*sizeof(float));

		// update internal state:
		numFrames_ += 1;
		dataSizeFloats_ += numValuesPerFrame;

		// Note: Header is _NOT_ updated (instead done on close()).

		// update in-memory table (will be written to file on close()):
		if (numValuesPerFrame_ == 0)
			sizeTable_.push_back(numValuesPerFrame);
	}

	void MatrixDataFileWrite::flush()
	{
		stream_.flush();
	}
}

